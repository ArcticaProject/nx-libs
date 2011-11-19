/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
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

#include "ClientStore.h"

//
// Cached request classes.
//

#include "ChangeProperty.h"
#include "SendEvent.h"
#include "CreateGC.h"
#include "ChangeGC.h"
#include "CreatePixmap.h"
#include "SetClipRectangles.h"
#include "CopyArea.h"
#include "PolyLine.h"
#include "PolySegment.h"
#include "PolyFillRectangle.h"
#include "PutImage.h"
#include "TranslateCoords.h"
#include "GetImage.h"
#include "ClearArea.h"
#include "ConfigureWindow.h"
#include "ShapeExtension.h"
#include "RenderExtension.h"
#include "PolyText8.h"
#include "PolyText16.h"
#include "ImageText8.h"
#include "ImageText16.h"
#include "PolyPoint.h"
#include "PolyFillArc.h"
#include "PolyArc.h"
#include "FillPoly.h"
#include "InternAtom.h"
#include "GetProperty.h"
#include "SetUnpackGeometry.h"
#include "SetUnpackColormap.h"
#include "SetUnpackAlpha.h"
#include "PutPackedImage.h"
#include "GenericRequest.h"

#include "ChangeGCCompat.h"
#include "CreatePixmapCompat.h"
#include "SetUnpackColormapCompat.h"
#include "SetUnpackAlphaCompat.h"

//
// Set the verbosity level.
//

#define WARNING
#define PANIC
#undef  TEST

ClientStore::ClientStore(StaticCompressor *compressor)

  : compressor_(compressor)
{
  if (logofs == NULL)
  {
    logofs = &cout;
  }

  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    requests_[i] = NULL;
  }

  requests_[X_ChangeProperty]    = new ChangePropertyStore();
  requests_[X_SendEvent]         = new SendEventStore();
  requests_[X_CreateGC]          = new CreateGCStore();
  requests_[X_SetClipRectangles] = new SetClipRectanglesStore();
  requests_[X_CopyArea]          = new CopyAreaStore();
  requests_[X_PolyLine]          = new PolyLineStore();
  requests_[X_PolySegment]       = new PolySegmentStore();
  requests_[X_PolyFillRectangle] = new PolyFillRectangleStore();
  requests_[X_PutImage]          = new PutImageStore(compressor);
  requests_[X_TranslateCoords]   = new TranslateCoordsStore();
  requests_[X_GetImage]          = new GetImageStore();
  requests_[X_ClearArea]         = new ClearAreaStore();
  requests_[X_ConfigureWindow]   = new ConfigureWindowStore();
  requests_[X_PolyText8]         = new PolyText8Store();
  requests_[X_PolyText16]        = new PolyText16Store();
  requests_[X_ImageText8]        = new ImageText8Store();
  requests_[X_ImageText16]       = new ImageText16Store();
  requests_[X_PolyPoint]         = new PolyPointStore();
  requests_[X_PolyFillArc]       = new PolyFillArcStore();
  requests_[X_PolyArc]           = new PolyArcStore();
  requests_[X_FillPoly]          = new FillPolyStore();
  requests_[X_InternAtom]        = new InternAtomStore();
  requests_[X_GetProperty]       = new GetPropertyStore();

  requests_[X_NXInternalShapeExtension]  = new ShapeExtensionStore(compressor);
  requests_[X_NXInternalGenericRequest]  = new GenericRequestStore(compressor);
  requests_[X_NXInternalRenderExtension] = new RenderExtensionStore(compressor);
  requests_[X_NXSetUnpackGeometry]       = new SetUnpackGeometryStore(compressor);
  requests_[X_NXPutPackedImage]          = new PutPackedImageStore(compressor);

  if (control -> isProtoStep7() == 1)
  {
    requests_[X_ChangeGC]            = new ChangeGCStore();
    requests_[X_CreatePixmap]        = new CreatePixmapStore();
    requests_[X_NXSetUnpackColormap] = new SetUnpackColormapStore(compressor);
    requests_[X_NXSetUnpackAlpha]    = new SetUnpackAlphaStore(compressor);
  }
  else
  {
    requests_[X_ChangeGC]            = new ChangeGCCompatStore();
    requests_[X_CreatePixmap]        = new CreatePixmapCompatStore();
    requests_[X_NXSetUnpackColormap] = new SetUnpackColormapCompatStore(compressor);
    requests_[X_NXSetUnpackAlpha]    = new SetUnpackAlphaCompatStore(compressor);
  }

  for (int i = 0; i < CHANNEL_STORE_RESOURCE_LIMIT; i++)
  {
    splits_[i] = NULL;
  }

  commits_ = new CommitStore(compressor);
}

ClientStore::~ClientStore()
{
  if (logofs == NULL)
  {
    logofs = &cout;
  }

  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    delete requests_[i];
  }

  for (int i = 0; i < CHANNEL_STORE_RESOURCE_LIMIT; i++)
  {
    delete splits_[i];
  }

  delete commits_;
}

int ClientStore::saveRequestStores(ostream *cachefs, md5_state_t *md5StateStream,
                                       md5_state_t *md5StateClient, T_checksum_action checksumAction,
                                           T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (requests_[i] != NULL &&
            requests_[i] -> saveStore(cachefs, md5StateStream, md5StateClient,
                                          checksumAction, dataAction,
                                              storeBigEndian()) < 0)
    {
      #ifdef WARNING
      *logofs << "ClientStore: WARNING! Error saving request store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      cerr << "Warning" << ": Error saving request store "
           << "for opcode '" << (unsigned int) i << "'.\n";

      return -1;
    }
  }

  return 1;
}

int ClientStore::loadRequestStores(istream *cachefs, md5_state_t *md5StateStream,
                                       T_checksum_action checksumAction, T_data_action dataAction) const
{
  for (int i = 0; i < CHANNEL_STORE_OPCODE_LIMIT; i++)
  {
    if (requests_[i] != NULL &&
            requests_[i] -> loadStore(cachefs, md5StateStream,
                                          checksumAction, dataAction,
                                              storeBigEndian()) < 0)
    {
      #ifdef WARNING
      *logofs << "ClientStore: WARNING! Error loading request store "
              << "for OPCODE#" << (unsigned int) i << ".\n"
              << logofs_flush;
      #endif

      return -1;
    }
  }

  return 1;
}

void ClientStore::dumpSplitStores() const
{
  for (int i = 0; i < CHANNEL_STORE_RESOURCE_LIMIT; i++)
  {
    if (splits_[i] != NULL)
    {
      splits_[i] -> dump();
    }
  }

  if ((getSplitTotalSize() != 0 && getSplitTotalStorageSize() == 0) ||
          (getSplitTotalSize() == 0 && getSplitTotalStorageSize() != 0))
  {
    #ifdef PANIC
    *logofs << "ClientStore: PANIC! Inconsistency detected "
            << "while handling the split stores.\n"
            << logofs_flush;
    #endif

    HandleCleanup();
  }
}
