
/*

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nx-X11/Xauth.h>
#include <stdlib.h>
#include <errno.h>

static int
read_short (unsigned short *shortp, FILE *file)
{
    unsigned char   file_short[2];

    /*
     * Added a check on EINTR to prevent the fread() call to be
     * interrupted by any signal not blocked by OsBlockSignals().
     */

    for (;;) {
        if (fread ((char *) file_short, (int) sizeof (file_short), 1, file) != 1) {
            if (errno == EINTR && ferror (file)) {
                perror ("Reading from auth file");
                clearerr (file);
                continue;
            }
            return 0;
        }
        break;
    }
    *shortp = file_short[0] * 256 + file_short[1];
    return 1;
}

static int
read_counted_string (unsigned short *countp, char **stringp, FILE *file)
{
    unsigned short  len;
    char	    *data;

    if (read_short (&len, file) == 0)
	return 0;
    if (len == 0) {
	data = 0;
    } else {
    	data = malloc ((unsigned) len);
    	if (!data)
	    return 0;
        for (;;)
        {
            if (fread (data, (int) sizeof (char), (int) len, file) != len)
            {
                if (errno == EINTR && ferror (file))
                {
                    perror ("Reading from auth file");
                    clearerr (file);
                    continue;
                }
	        bzero (data, len);
	        free (data);
	        return 0;
    	    }
            break;
        }
    }
    *stringp = data;
    *countp = len;
    return 1;
}

Xauth *
XauReadAuth (auth_file)
FILE	*auth_file;
{
    Xauth   local;
    Xauth   *ret;

    if (read_short (&local.family, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.address_length, &local.address, auth_file) == 0)
	return 0;
    if (read_counted_string (&local.number_length, &local.number, auth_file) == 0) {
	if (local.address) free (local.address);
	return 0;
    }
    if (read_counted_string (&local.name_length, &local.name, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	return 0;
    }
    if (read_counted_string (&local.data_length, &local.data, auth_file) == 0) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	return 0;
    }
    ret = (Xauth *) malloc (sizeof (Xauth));
    if (!ret) {
	if (local.address) free (local.address);
	if (local.number) free (local.number);
	if (local.name) free (local.name);
	if (local.data) {
	    bzero (local.data, local.data_length);
	    free (local.data);
	}
	return 0;
    }
    *ret = local;
    return ret;
}
