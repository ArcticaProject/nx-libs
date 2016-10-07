/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2011 NoMachine (http://www.nomachine.com)          */
/* Copyright (c) 2008-2014 Oleksandr Shneyder <o.shneyder@phoca-gmbh.de>  */
/* Copyright (c) 2011-2016 Mike Gabriel <mike.gabriel@das-netzwerkteam.de>*/
/* Copyright (c) 2014-2016 Mihai Moldovan <ionic@ionic.de>                */
/* Copyright (c) 2014-2016 Ulrich Sibiller <uli42@gmx.de>                 */
/* Copyright (c) 2015-2016 Qindel Group (http://www.qindel.com)           */
/*                                                                        */
/* nx-X11, NX protocol compression and NX extensions to this software     */
/* are copyright of the aforementioned persons and companies.             */
/*                                                                        */
/* Redistribution and use of the present software is allowed according    */
/* to terms specified in the file LICENSE which comes in the source       */
/* distribution.                                                          */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/* NOTE: This software has received contributions from various other      */
/* contributors, only the core maintainers and supporters are listed as   */
/* copyright holders. Please contact us, if you feel you should be listed */
/* as copyright holder, as well.                                          */
/*                                                                        */
/**************************************************************************/

/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"

RESTYPE RRModeType;

static Bool
RRModeEqual(xRRModeInfo * a, xRRModeInfo * b)
{
    if (a->width != b->width)
        return FALSE;
    if (a->height != b->height)
        return FALSE;
    if (a->dotClock != b->dotClock)
        return FALSE;
    if (a->hSyncStart != b->hSyncStart)
        return FALSE;
    if (a->hSyncEnd != b->hSyncEnd)
        return FALSE;
    if (a->hTotal != b->hTotal)
        return FALSE;
    if (a->hSkew != b->hSkew)
        return FALSE;
    if (a->vSyncStart != b->vSyncStart)
        return FALSE;
    if (a->vSyncEnd != b->vSyncEnd)
        return FALSE;
    if (a->vTotal != b->vTotal)
        return FALSE;
    if (a->nameLength != b->nameLength)
        return FALSE;
    if (a->modeFlags != b->modeFlags)
        return FALSE;
    return TRUE;
}

/*
 * Keep a list so it's easy to find modes in the resource database.
 */
static int num_modes;
static RRModePtr *modes;

static RRModePtr
RRModeCreate(xRRModeInfo * modeInfo, const char *name, ScreenPtr userScreen)
{
    RRModePtr mode, *newModes;

    if (!RRInit())
        return NULL;

    mode = malloc(sizeof(RRModeRec) + modeInfo->nameLength + 1);
    if (!mode)
        return NULL;
    mode->refcnt = 1;
    mode->mode = *modeInfo;
    mode->name = (char *) (mode + 1);
    memcpy(mode->name, name, modeInfo->nameLength);
    mode->name[modeInfo->nameLength] = '\0';
    mode->userScreen = userScreen;

    if (num_modes)
#ifndef NXAGENT_SERVER
        newModes = reallocarray(modes, num_modes + 1, sizeof(RRModePtr));
#else                           /* !defined(NXAGENT_SERVER) */
        newModes = realloc(modes, (num_modes + 1) * sizeof(RRModePtr));
#endif                          /* !defined(NXAGENT_SERVER) */

    else
        newModes = malloc(sizeof(RRModePtr));

    if (!newModes) {
        free(mode);
        return NULL;
    }

    mode->mode.id = FakeClientID(0);
    if (!AddResource(mode->mode.id, RRModeType, (void *) mode)) {
        free(newModes);
        return NULL;
    }
    modes = newModes;
    modes[num_modes++] = mode;

    /*
     * give the caller a reference to this mode
     */
    ++mode->refcnt;
#ifdef DEBUG
    fprintf(stderr,
            "RRModeCreate: num_modes [%d] new mode [%s] ([%p]) refcnt [%d]\n",
            num_modes, mode->name, mode, mode->refcnt);
#endif
    return mode;
}

static RRModePtr
RRModeFindByName(const char *name, CARD16 nameLength)
{
    int i;
    RRModePtr mode;

    for (i = 0; i < num_modes; i++) {
        mode = modes[i];
        if (mode->mode.nameLength == nameLength &&
            !memcmp(name, mode->name, nameLength)) {
            return mode;
        }
    }
    return NULL;
}

RRModePtr
RRModeGet(xRRModeInfo * modeInfo, const char *name)
{
    int i;

    for (i = 0; i < num_modes; i++) {
        RRModePtr mode = modes[i];

        if (RRModeEqual(&mode->mode, modeInfo) &&
            !memcmp(name, mode->name, modeInfo->nameLength)) {
            ++mode->refcnt;
#ifdef DEBUG
            fprintf(stderr,
                    "RRModeGet: return existing mode [%s] ([%p]) refcnt [%d]\n",
                    mode->name, mode, mode->refcnt);
#endif
            return mode;
        }
    }

#ifdef DEBUG
    {
        RRModePtr mode = RRModeCreate(modeInfo, name, NULL);

        fprintf(stderr, "RRModeGet: return new mode [%s] ([%p]) refcnt [%d]\n",
                mode->name, mode, mode->refcnt);
        return mode;
    }
#else
    return RRModeCreate(modeInfo, name, NULL);
#endif
}

static RRModePtr
RRModeCreateUser(ScreenPtr pScreen,
                 xRRModeInfo * modeInfo, const char *name, int *error)
{
    RRModePtr mode;

    mode = RRModeFindByName(name, modeInfo->nameLength);
    if (mode) {
        *error = BadName;
        return NULL;
    }

    mode = RRModeCreate(modeInfo, name, pScreen);
    if (!mode) {
        *error = BadAlloc;
        return NULL;
    }
    *error = Success;
    return mode;
}

RRModePtr *
RRModesForScreen(ScreenPtr pScreen, int *num_ret)
{
    rrScrPriv(pScreen);
    int o, c, m;
    RRModePtr *screen_modes;
    int num_screen_modes = 0;

#ifndef NXAGENT_SERVER
    screen_modes = xallocarray((num_modes ? num_modes : 1), sizeof(RRModePtr));
#else                           /* !defined(NXAGENT_SERVER) */
    screen_modes = malloc((num_modes ? num_modes : 1) * sizeof(RRModePtr));
#endif                          /* !defined(NXAGENT_SERVER) */
    if (!screen_modes)
        return NULL;

    /*
     * Add modes from all outputs
     */
    for (o = 0; o < pScrPriv->numOutputs; o++) {
        RROutputPtr output = pScrPriv->outputs[o];
        int n;

        for (m = 0; m < output->numModes + output->numUserModes; m++) {
            RRModePtr mode = (m < output->numModes ?
                              output->modes[m] :
                              output->userModes[m - output->numModes]);
            for (n = 0; n < num_screen_modes; n++)
                if (screen_modes[n] == mode)
                    break;
            if (n == num_screen_modes)
                screen_modes[num_screen_modes++] = mode;
        }
    }
    /*
     * Add modes from all crtcs. The goal is to
     * make sure all available and active modes
     * are visible to the client
     */
    for (c = 0; c < pScrPriv->numCrtcs; c++) {
        RRCrtcPtr crtc = pScrPriv->crtcs[c];
        RRModePtr mode = crtc->mode;
        int n;

        if (!mode)
            continue;
        for (n = 0; n < num_screen_modes; n++)
            if (screen_modes[n] == mode)
                break;
        if (n == num_screen_modes)
            screen_modes[num_screen_modes++] = mode;
    }
    /*
     * Add all user modes for this screen
     */
    for (m = 0; m < num_modes; m++) {
        RRModePtr mode = modes[m];
        int n;

        if (mode->userScreen != pScreen)
            continue;
        for (n = 0; n < num_screen_modes; n++)
            if (screen_modes[n] == mode)
                break;
        if (n == num_screen_modes)
            screen_modes[num_screen_modes++] = mode;
    }

    *num_ret = num_screen_modes;
    return screen_modes;
}

void
RRModeDestroy(RRModePtr mode)
{
    int m;

    if (--mode->refcnt > 0)
        return;
    for (m = 0; m < num_modes; m++) {
        if (modes[m] == mode) {
            memmove(modes + m, modes + m + 1,
                    (num_modes - m - 1) * sizeof(RRModePtr));
            num_modes--;
            if (!num_modes) {
                free(modes);
                modes = NULL;
            }
            break;
        }
    }

    free(mode);
}

static int
RRModeDestroyResource(void *value, XID pid)
{
    RRModeDestroy((RRModePtr) value);
    return 1;
}

/*
 * Initialize mode type
 */
Bool
RRModeInit(void)
{
    assert(num_modes == 0);
    assert(modes == NULL);
    RRModeType = CreateNewResourceType(RRModeDestroyResource
#ifndef NXAGENT_SERVER
                                       , "MODE"
#endif
        );
    if (!RRModeType)
        return FALSE;

    return TRUE;
}

/*
 * Initialize mode type error value
 */
void
RRModeInitErrorValue(void)
{
#ifndef NXAGENT_SERVER
    SetResourceTypeErrorValue(RRModeType, RRErrorBase + BadRRMode);
#endif
}

int
ProcRRCreateMode(ClientPtr client)
{
    REQUEST(xRRCreateModeReq);
    xRRCreateModeReply rep;
    WindowPtr pWin;
    ScreenPtr pScreen;
    xRRModeInfo *modeInfo;
    long units_after;
    char *name;
    int error, rc;
    RRModePtr mode;

    REQUEST_AT_LEAST_SIZE(xRRCreateModeReq);
#ifndef NXAGENT_SERVER
    rc = dixLookupWindow(&pWin, stuff->window, client, DixGetAttrAccess);
#else
    pWin = SecurityLookupWindow(stuff->window, client, SecurityReadAccess);
    rc = pWin ? Success : BadWindow;
#endif
    if (rc != Success)
        return rc;

    pScreen = pWin->drawable.pScreen;

    modeInfo = &stuff->modeInfo;
    name = (char *) (stuff + 1);
    units_after = (stuff->length - bytes_to_int32(sizeof(xRRCreateModeReq)));

    /* check to make sure requested name fits within the data provided */
    if (bytes_to_int32(modeInfo->nameLength) > units_after)
        return BadLength;

    mode = RRModeCreateUser(pScreen, modeInfo, name, &error);
    if (!mode)
        return error;

    rep = (xRRCreateModeReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = 0,
        .mode = mode->mode.id
    };
    if (client->swapped) {
        swaps(&rep.sequenceNumber);
        swapl(&rep.length);
        swapl(&rep.mode);
    }
    WriteToClient(client, sizeof(xRRCreateModeReply), &rep);
    /* Drop out reference to this mode */
    RRModeDestroy(mode);
    return Success;
}

int
ProcRRDestroyMode(ClientPtr client)
{
    REQUEST(xRRDestroyModeReq);
    RRModePtr mode;

    REQUEST_SIZE_MATCH(xRRDestroyModeReq);
    VERIFY_RR_MODE(stuff->mode, mode, DixDestroyAccess);

    if (!mode->userScreen)
        return BadMatch;
    if (mode->refcnt > 1)
        return BadAccess;
    FreeResource(stuff->mode, 0);
    return Success;
}

int
ProcRRAddOutputMode(ClientPtr client)
{
    REQUEST(xRRAddOutputModeReq);
    RRModePtr mode;
    RROutputPtr output;

    REQUEST_SIZE_MATCH(xRRAddOutputModeReq);
    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);
    VERIFY_RR_MODE(stuff->mode, mode, DixUseAccess);

    return RROutputAddUserMode(output, mode);
}

int
ProcRRDeleteOutputMode(ClientPtr client)
{
    REQUEST(xRRDeleteOutputModeReq);
    RRModePtr mode;
    RROutputPtr output;

    REQUEST_SIZE_MATCH(xRRDeleteOutputModeReq);
    VERIFY_RR_OUTPUT(stuff->output, output, DixReadAccess);
    VERIFY_RR_MODE(stuff->mode, mode, DixUseAccess);

    return RROutputDeleteUserMode(output, mode);
}
