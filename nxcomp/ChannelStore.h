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

#ifndef ChannelStore_H
#define ChannelStore_H

//
// One message store for each opcode.
//

#define CHANNEL_STORE_OPCODE_LIMIT        256

//
// One split store for each resource.
//

#define CHANNEL_STORE_RESOURCE_LIMIT      256

class ChannelStore
{
  public:

  ChannelStore()
  {
  }

  virtual ~ChannelStore()
  {
  }
};

#endif /* ChannelStore_H */
