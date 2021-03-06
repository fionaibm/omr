/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2000, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/

#include "codegen/OMRRealRegister.hpp"  // for RealRegister, etc

#include <stddef.h>                              // for NULL
#include <stdint.h>                              // for uint16_t, int32_t, etc
#include "codegen/CodeGenerator.hpp"             // for CodeGenerator
#include "codegen/RealRegister.hpp"              // for RealRegister
#include "codegen/Register.hpp"                  // for Register
#include "codegen/RegisterConstants.hpp"         // for TR_RegisterKinds, etc
#include "infra/Flags.hpp"                       // for flags8_t

OMR::RealRegister::RealRegister(TR::CodeGenerator *cg, RegNum n) :
                                       TR::Register(),
                                       _weight(0),
                                       _state(Free),
                                       _realRegFlags(0),
                                       _registerNumber(n),
                                       _registerMask((RegMask)0),
                                       _cg(cg){}

OMR::RealRegister::RealRegister(TR_RegisterKinds rk,
                uint16_t         w,
                RegState        s,
                uint16_t         a,
                RegNum      n,
                RegMask  m,
                TR::CodeGenerator *cg) : TR::Register(rk, a),
                                      _weight(w),
                                      _state(s),
                                      _realRegFlags(0),
                                      _registerNumber(n),
                                      _registerMask(m),
                                      _cg(cg) {}

TR::RealRegister*
OMR::RealRegister::self()
    {
    return static_cast<TR::RealRegister*>(this);
    }

TR::Register *OMR::RealRegister::setAssignedRegister(TR::Register *r)
    {
    if(r && r != self()->getAssignedRegister())
        {
        if(self()->getIsAssignedOnce())
            self()->setIsAssignedMoreThanOnce();
        else
            self()->setIsAssignedOnce();
        }
    return OMR::Register::setAssignedRegister(r);
    }

TR::RealRegister::RegState
OMR::RealRegister::setState(TR::RealRegister::RegState s, bool assignedToDummy)
   {
      // can not overwrite locked registers, use resetState() instead
      if (_state == Locked) return _state;

      _state = s;

      if (s == Assigned && !assignedToDummy)
         self()->setHasBeenAssignedInMethod(true);
      else if (s == Unlatched)
         _cg->addToUnlatchedRegisterList(self());

//      else if (s == Free)
//         setHasBeenAssignedInMethod(false);

      return _state;
   }

bool
OMR::RealRegister::setHasBeenAssignedInMethod(bool b)
   {
   // we use _realRegFlags flag to indicate whether a register was assigned in this method,
   // so that when creating prologue, we know which registers need to be preserved
   // and save only those that has been used in this method.
   // note that this is different from the assigned state, the _state member is
   // the current state at a particular moment. this flag is to indicate if it was
   // ever assigned in this method at some point.
   if (b) _realRegFlags.set(isAssigned);
   else _realRegFlags.reset(isAssigned);

   return b;
   }

void
OMR::RealRegister::block()
   {
   if (self()->getState() == TR::RealRegister::Assigned || self()->getState() == TR::RealRegister::Free)
      {
      self()->setState(TR::RealRegister::Blocked);
      }
   }

void
OMR::RealRegister::unblock()
   {
   if (self()->getState() == TR::RealRegister::Blocked)
      {
      if (self()->getAssignedRegister())
         {
         self()->setState(TR::RealRegister::Assigned, self()->getAssignedRegister()->isPlaceholderReg());
         }
      else
         {
         self()->setState(TR::RealRegister::Free);
         }
      }
   }

TR::RealRegister *
OMR::RealRegister::getRealRegister()
   {
   return self();
   }

TR::Register *
OMR::RealRegister::getRegister()
   {
   return NULL;
   }

// Return the position of the rightmost set bit in the mask.  The first bit
// position is 0.
//
int32_t
OMR::RealRegister::getBitPosInMask(TR_RegisterMask mask)
   {
   int8_t reg = 0;
   while (mask)
      {
      if (mask & 1) return reg;
      mask >>= 1;
      reg++;
      }
   return -1;
   }
