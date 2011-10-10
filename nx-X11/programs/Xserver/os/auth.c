/* $Xorg: auth.c,v 1.5 2001/02/09 02:05:23 xorgcvs Exp $ */
/*

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: auth.c,v 1.13 2003/04/27 21:31:08 herrb Exp $ */

/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NX-X11, NX protocol compression and NX extensions to this software     */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

/*
 * authorization hooks for the server
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef K5AUTH
# include   <krb5/krb5.h>
#endif
# include   <X11/X.h>
# include   <X11/Xauth.h>
# include   "misc.h"
# include   "osdep.h"
# include   "dixstruct.h"
# include   <sys/types.h>
# include   <sys/stat.h>
#ifdef XCSECURITY
#define _SECURITY_SERVER
# include   <X11/extensions/security.h>
#endif
#ifdef WIN32
#include    <X11/Xw32defs.h>
#endif

struct protocol {
    unsigned short   name_length;
    char    *name;
    AuthAddCFunc	Add;	/* new authorization data */
    AuthCheckFunc	Check;	/* verify client authorization data */
    AuthRstCFunc	Reset;	/* delete all authorization data entries */
    AuthToIDFunc	ToID;	/* convert cookie to ID */
    AuthFromIDFunc	FromID;	/* convert ID to cookie */
    AuthRemCFunc	Remove;	/* remove a specific cookie */
#ifdef XCSECURITY
    AuthGenCFunc	Generate;
#endif
};

static struct protocol   protocols[] = {
{   (unsigned short) 18,    "MIT-MAGIC-COOKIE-1",
		MitAddCookie,	MitCheckCookie,	MitResetCookie,
		MitToID,	MitFromID,	MitRemoveCookie,
#ifdef XCSECURITY
		MitGenerateCookie
#endif
},
#ifdef HASXDMAUTH
{   (unsigned short) 19,    "XDM-AUTHORIZATION-1",
		XdmAddCookie,	XdmCheckCookie,	XdmResetCookie,
		XdmToID,	XdmFromID,	XdmRemoveCookie,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef SECURE_RPC
{   (unsigned short) 9,    "SUN-DES-1",
		SecureRPCAdd,	SecureRPCCheck,	SecureRPCReset,
		SecureRPCToID,	SecureRPCFromID,SecureRPCRemove,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef K5AUTH
{   (unsigned short) 14, "MIT-KERBEROS-5",
		K5Add, K5Check, K5Reset,
		K5ToID, K5FromID, K5Remove,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef XCSECURITY
{   (unsigned short) XSecurityAuthorizationNameLen,
	XSecurityAuthorizationName,
		NULL, AuthSecurityCheck, NULL,
		NULL, NULL, NULL,
		NULL
},
#endif
};

# define NUM_AUTHORIZATION  (sizeof (protocols) /\
			     sizeof (struct protocol))

/*
 * Initialize all classes of authorization by reading the
 * specified authorization file
 */

static char *authorization_file = (char *)NULL;

static Bool ShouldLoadAuth = TRUE;

void
InitAuthorization (char *file_name)
{
#ifdef __sun 
    char * envBuffer;
#endif
    authorization_file = file_name;
#ifdef NX_TRANS_AUTH
#ifdef NX_TRANS_TEST
    fprintf(stderr, "InitAuthorization: Going to propagate auth file '%s' to the environment.\n",
                authorization_file);
#endif
#ifdef __sun 
    envBuffer = malloc(15+strlen(authorization_file));
    sprintf(envBuffer,"NX_XAUTHORITY=%s",authorization_file);
    putenv(envBuffer);
#else
    setenv("NX_XAUTHORITY", authorization_file, 1);
#endif
#endif

}

static int
LoadAuthorization (void)
{
    FILE    *f;
    Xauth   *auth;
    int	    i;
    int	    count = 0;

    ShouldLoadAuth = FALSE;
    if (!authorization_file)
	return 0;

#ifdef NX_TRANS_AUTH

    /*
     * We think that the way LoadAuthorization() is working is wrong.
     * It doesn't reset the list of stored authorizations before reading
     * the new cookies. Our take is that if a new auth file is to be
     * read, the only cookies that are to be accepted are those that are
     * in the new file, not those in the file -plus- those that have
     * been in the file in the past. Furthermore, if the list can't be
     * read or it is empty, it should assume that it ignores which co-
     * okies are valid and thus it should disable any access. Your mile-
     * age can vary. A less draconian approach could be to leave the old
     * cookies if the file can't be read and remove them only if the
     * file is empty.
     *
     * Adding the cookies without removing the old values for the same
     * protocol has an important implication. If an user shares the co-
     * okie with somebody and later wants to revoke the access to the
     * display, changing the cookie will not work. This is especially
     * important with NX. For security reasons, after reconnecting the
     * session to a different display, it is advisable to generate a
     * new set of cookies, but doing that it is useless with the current
     * code, as the old cookies are going to be still accepted. On the
     * same topic, consider that once an user has got access to the X
     * server, he/she can freely enable host authentication from any
     * host, so the safe behaviour should be to reset the host based
     * authenthication at least at reconnection, and keep as valid only
     * the cookies that are actually in the file. This behaviour would
     * surely break many applications, among them a SSH connection run
     * inside a NX session, as ssh -X reads the cookie for the display
     * only at session startup and does not read the cookies again
     * when the auth file is changed.
     *
     * Another bug (or feature, depending on how you want to consider
     * it) is that if the authority file contains entries for different
     * displays (as it is the norm when the authority file is the default
     * .Xauthority in the user's home), the server will match -any- of
     * the cookies, even cookies that are not for its own display. This 
     * means that you have be careful when passing an authority file to
     * nxagent or Xnest and maybe keep separate files for letting nxagent
     * find the cookie to be used to connect to the remote display and
     * for letting it find what cookies to accept. If the file is the
     * same, clients will be able to connect to nxagent with both the
     * cookies.
     */

#ifdef NX_TRANS_AUTH_RESET

    #ifdef NX_TRANS_TEST
    fprintf(stderr, "LoadAuthorization: Resetting authorization info.\n");
    #endif

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
        if (protocols[i].Reset) {
            (*protocols[i].Reset) ();
        }
    }

#endif

#endif /* #ifdef NX_TRANS_AUTH */

    f = Fopen (authorization_file, "r");
    if (!f)
	return -1;

    while ((auth = XauReadAuth (f)) != 0) {
	for (i = 0; i < NUM_AUTHORIZATION; i++) {
	    if (protocols[i].name_length == auth->name_length &&
		memcmp (protocols[i].name, auth->name, (int) auth->name_length) == 0 &&
		protocols[i].Add)
	    {
#ifdef NX_TRANS_AUTH

                #ifdef NX_TRANS_TEST
                fprintf(stderr, "LoadAuthorization: Adding new record from file [%s].\n",
                            authorization_file);
                #endif

#endif
		++count;
		(*protocols[i].Add) (auth->data_length, auth->data,
					 FakeClientID(0));
	    }
	}
	XauDisposeAuth (auth);
    }

#ifdef NX_TRANS_AUTH

    if (count == 0)
    {
      fprintf(stderr, "Warning: No authorization record could be read from file '%s'.\n",
                  authorization_file);

      fprintf(stderr, "Warning: Please, create a valid authorization cookie using the command\n"
                  "Warning: 'xauth -f %s add <display> MIT-MAGIC-COOKIE-1 <cookie>'.\n",
                      authorization_file);
    }

#endif

#ifdef NX_TRANS_AUTH
    if (Fclose (f) != 0)
    {
        /*
         * If the Fclose() fails, for example because of a signal,
         * it's advisable to return the number of protocols read,
         * if any, or otherwise the server would believe that no
         * cookie is valid and eventually fall back to host based
         * authentication. Note anyway that the new code in Check-
         * Authorization() doesn't care the return value and gives
         * a chance to the function to check the file at the next
         * connection.
         */

        if (count > 0)
        {
            return count;
        }
        else
        {
            return -1;
        }
    }
#else
    Fclose (f);
#endif
    return count;
}

#ifdef XDMCP
/*
 * XdmcpInit calls this function to discover all authorization
 * schemes supported by the display
 */
void
RegisterAuthorizations (void)
{
    int	    i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
	XdmcpRegisterAuthorization (protocols[i].name,
				    (int)protocols[i].name_length);
}
#endif

XID
CheckAuthorization (
    unsigned int name_length,
    char	*name,
    unsigned int data_length,
    char	*data,
    ClientPtr client,
    char	**reason)	/* failure message.  NULL for default msg */
{
    int	i;
    struct stat buf;
    static time_t lastmod = 0;

    #ifndef NX_TRANS_AUTH
    static Bool loaded = FALSE;
    #endif

    if (!authorization_file || stat(authorization_file, &buf))
    {
	if (lastmod != 0) {
	    lastmod = 0;
	    ShouldLoadAuth = TRUE;	/* stat lost, so force reload */
	}
    }
    else if (buf.st_mtime > lastmod)
    {
	lastmod = buf.st_mtime;
	ShouldLoadAuth = TRUE;
    }
    if (ShouldLoadAuth)
    {
	int loadauth = LoadAuthorization();

	/*
	 * If the authorization file has at least one entry for this server,
	 * disable local host access. (loadauth > 0)
	 *
	 * If there are zero entries (either initially or when the
	 * authorization file is later reloaded), or if a valid
	 * authorization file was never loaded, enable local host access.
	 * (loadauth == 0 || !loaded)
	 *
	 * If the authorization file was loaded initially (with valid
	 * entries for this server), and reloading it later fails, don't
	 * change anything. (loadauth == -1 && loaded)
	 */

#ifdef NX_TRANS_AUTH

        /*
         * The implementation of CheckAuthorization() was changed. The way
         * the auth file was handled previously was questionable and could
         * open the way to a vast array of security problems. There might be
         * ways for an attacker to prevent the server from reading the file
         * and it was enough for the server to fail reading the file once
         * (because of a not blocked signal, for example) to leave the dis-
         * play open to all the users running a session on the same terminal
         * server.
         *
         * In NX we want to have only two cases: either we have to check an
         * authorization file or we don't. In the first case we need to do our
         * best to read the file at any new client access and never fall back
         * to host based authentication. Falling back to local host access has
         * no way back, as it will always take precedence over the auth cookie
         * (unless the user explicitly disables, one by one, all the rules
         * allowing local access, if and only if he/she becomes aware of the
         * problem). In the second case we assume that user doesn't care secu-
         * rity and so allow unrestricted access from the local machine.
         */

        #ifdef NX_TRANS_TEST
        fprintf(stderr, "CheckAuthorization: Going to set authorization with loadauth [%d].\n",
                    loadauth);
        #endif

        if (authorization_file)
        {
            #ifdef NX_TRANS_TEST
            fprintf(stderr, "CheckAuthorization: Disabling local host access.\n");
            #endif

            DisableLocalHost();
        }
        else
        {
            /*
             * Enable host-based authentication only if
             * the authorization file was not specified
             * either on the command line or in the env-
             * ironment.
             */

            #ifdef NX_TRANS_TEST
            fprintf(stderr, "CheckAuthorization: Enabling local host access.\n");
            #endif

            EnableLocalHost();
        }

        /*
         * Avoid the 'unused variable' warning.
         */

        loadauth = loadauth;

#else /* #ifdef NX_TRANS_AUTH */

	if (loadauth > 0)
	{
	    DisableLocalHost(); /* got at least one */
	    loaded = TRUE;
	}
	else if (loadauth == 0 || !loaded)
	    EnableLocalHost ();

#endif /* #ifdef NX_TRANS_AUTH */
    }
    if (name_length) {
	for (i = 0; i < NUM_AUTHORIZATION; i++) {
	    if (protocols[i].name_length == name_length &&
		memcmp (protocols[i].name, name, (int) name_length) == 0)
	    {
		return (*protocols[i].Check) (data_length, data, client, reason);
	    }
	    *reason = "Protocol not supported by server\n";
	}
    } else *reason = "No protocol specified\n";
    return (XID) ~0L;
}

void
ResetAuthorization (void)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
	if (protocols[i].Reset)
	    (*protocols[i].Reset)();
    ShouldLoadAuth = TRUE;
}

XID
AuthorizationToID (
	unsigned short	name_length,
	char		*name,
	unsigned short	data_length,
	char		*data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].ToID)
    	{
	    return (*protocols[i].ToID) (data_length, data);
    	}
    }
    return (XID) ~0L;
}

int
AuthorizationFromID (
	XID 		id,
	unsigned short	*name_lenp,
	char		**namep,
	unsigned short	*data_lenp,
	char		**datap)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
	if (protocols[i].FromID &&
	    (*protocols[i].FromID) (id, data_lenp, datap)) {
	    *name_lenp = protocols[i].name_length;
	    *namep = protocols[i].name;
	    return 1;
	}
    }
    return 0;
}

int
RemoveAuthorization (
	unsigned short	name_length,
	char		*name,
	unsigned short	data_length,
	char		*data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Remove)
    	{
	    return (*protocols[i].Remove) (data_length, data);
    	}
    }
    return 0;
}

int
AddAuthorization (unsigned name_length, char *name, unsigned data_length, char *data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Add)
    	{
	    return (*protocols[i].Add) (data_length, data, FakeClientID(0));
    	}
    }
    return 0;
}

#ifdef XCSECURITY

XID
GenerateAuthorization(
	unsigned name_length,
	char	*name,
	unsigned data_length,
	char	*data,
	unsigned *data_length_return,
	char	**data_return)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Generate)
    	{
	    return (*protocols[i].Generate) (data_length, data,
			FakeClientID(0), data_length_return, data_return);
    	}
    }
    return -1;
}

/* A random number generator that is more unpredictable
   than that shipped with some systems.
   This code is taken from the C standard. */

static unsigned long int next = 1;

static int
xdm_rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

static void
xdm_srand(unsigned int seed)
{
    next = seed;
}

void
GenerateRandomData (int len, char *buf)
{
    static int seed;
    int value;
    int i;

    seed += GetTimeInMillis();
    xdm_srand (seed);
    for (i = 0; i < len; i++)
    {
	value = xdm_rand ();
	buf[i] ^= (value & 0xff00) >> 8;
    }

    /* XXX add getrusage, popen("ps -ale") */
}

#endif /* XCSECURITY */
