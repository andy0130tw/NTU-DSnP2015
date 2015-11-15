/****************************************************************************
  FileName     [ memCmd.cpp ]
  PackageName  [ mem ]
  Synopsis     [ Define memory test commands ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2007-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/
#include <iostream>
#include <iomanip>
#include "memCmd.h"
#include "memTest.h"
#include "cmdParser.h"
#include "util.h"

using namespace std;

extern MemTest mtest;  // defined in memTest.cpp

bool
initMemCmd()
{
   if (!(cmdMgr->regCmd("MTReset", 3, new MTResetCmd) &&
         cmdMgr->regCmd("MTNew", 3, new MTNewCmd) &&
         cmdMgr->regCmd("MTDelete", 3, new MTDeleteCmd) &&
         cmdMgr->regCmd("MTPrint", 3, new MTPrintCmd)
      )) {
      cerr << "Registering \"mem\" commands fails... exiting" << endl;
      return false;
   }
   return true;
}


//----------------------------------------------------------------------
//    MTReset [(size_t blockSize)]
//----------------------------------------------------------------------
CmdExecStatus
MTResetCmd::exec(const string& option)
{
   // check option
   string token;
   if (!CmdExec::lexSingleOption(option, token))
      return CMD_EXEC_ERROR;
   if (token.size()) {
      int b;
      if (!myStr2Int(token, b) || b < int(toSizeT(sizeof(MemTestObj)))) {
         cerr << "Illegal block size (" << token << ")!!" << endl;
         return CmdExec::errorOption(CMD_OPT_ILLEGAL, token);
      }
      #ifdef MEM_MGR_H
      mtest.reset(toSizeT(b));
      #else
      mtest.reset();
      #endif // MEM_MGR_H
   }
   else
      mtest.reset();
   return CMD_EXEC_DONE;
}

void
MTResetCmd::usage(ostream& os) const
{
   os << "Usage: MTReset [(size_t blockSize)]" << endl;
}

void
MTResetCmd::help() const
{
   cout << setw(15) << left << "MTReset: "
        << "(memory test) reset memory manager" << endl;
}


//----------------------------------------------------------------------
//    MTNew <(size_t numObjects)> [-Array (size_t arraySize)]
//----------------------------------------------------------------------
CmdExecStatus
MTNewCmd::exec(const string& option)
{
   // TODO
   // check options
   vector<string> tokens;
   int numObj = -1, arrSize = -1;

   // this should not fail
   if (!CmdExec::lexOptions(option, tokens))
      return CMD_EXEC_ERROR;

   size_t toklen = tokens.size();

   if (toklen < 1)
      return CmdExec::errorOption(CMD_OPT_MISSING, "");
   // if (toklen > 3)
      // return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[3]);

   for (size_t i = 0, n = tokens.size(); i < n; i++) {
      if (myStrNCmp("-Array", tokens[i], 2) == 0) {
         // repeated specifying
         if (arrSize >= 0)
            return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[i]);

         if (i + 1 >= toklen)
            return CmdExec::errorOption(CMD_OPT_MISSING, tokens[i]);
         // skip one field
         i++;
         if (!myStr2Int(tokens[i], arrSize) || arrSize <= 0)
            return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[i]);
      } else {
         // mandatory arguments already encountered
         if (numObj >= 0)
            return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[i]);
         if (!myStr2Int(tokens[i], numObj) || numObj <= 0)
            return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[i]);
      }
   }

   // check mandatory option is correctly specified
   if (numObj < 0)
      return CmdExec::errorOption(CMD_OPT_MISSING, "");

   try {
      if (arrSize > 0)
         mtest.newArrs(numObj, arrSize);
      else
         mtest.newObjs(numObj);
   } catch (bad_alloc) {
      return CMD_EXEC_ERROR;
   }

   return CMD_EXEC_DONE;
}

void
MTNewCmd::usage(ostream& os) const
{
   os << "Usage: MTNew <(size_t numObjects)> [-Array (size_t arraySize)]\n";
}

void
MTNewCmd::help() const
{
   cout << setw(15) << left << "MTNew: "
        << "(memory test) new objects" << endl;
}


//----------------------------------------------------------------------
//    MTDelete <-Index (size_t objId) | -Random (size_t numRandId)> [-Array]
//----------------------------------------------------------------------
CmdExecStatus
MTDeleteCmd::exec(const string& option)
{
   // TODO
   // check options
   vector<string> tokens;

   static const int DO_ARRAY =  1 << 2;
   static const int DO_INDEX =  1 << 1;
   static const int DO_RANDOM = 1 << 0;

   static const string TERM_OBJECT = "object";
   static const string TERM_ARRAY  = "array";

   // should rewrite this to record the position
   int inputFlag = 0;
   size_t optLen = -1, optRand = -1;

   // this can be stored in an union?!
   int objId = -1, numRandId = -1;

   // this should not fail
   if (!CmdExec::lexOptions(option, tokens))
      return CMD_EXEC_ERROR;

   size_t toklen = tokens.size();

   if (toklen < 1)
      return CmdExec::errorOption(CMD_OPT_MISSING, "");

   for (size_t i = 0, n = tokens.size(); i < n; i++) {
      if (myStrNCmp("-Array", tokens[i], 2) == 0) {
         if (inputFlag & DO_ARRAY)
            return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[i]);
         inputFlag |= DO_ARRAY;

      } else if (myStrNCmp("-Index", tokens[i], 2) == 0) {
         if (inputFlag & (DO_RANDOM | DO_INDEX))
            return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[i]);
         if (i + 1 >= toklen)
            return CmdExec::errorOption(CMD_OPT_MISSING, tokens[i]);
         i++;
         if (!myStr2Int(tokens[i], objId) || objId < 0) {
            return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[i]);
         }
         optLen = i;
         inputFlag |= DO_INDEX;
      } else if (myStrNCmp("-Random", tokens[i], 2) == 0) {
         if (inputFlag & (DO_RANDOM | DO_INDEX))
            return CmdExec::errorOption(CMD_OPT_EXTRA, tokens[i]);
         if (i + 1 >= toklen)
            return CmdExec::errorOption(CMD_OPT_MISSING, tokens[i]);
         i++;
         if (!myStr2Int(tokens[i], numRandId) || numRandId < 0) {
            return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[i]);
         }
         optRand = i;
         inputFlag |= DO_RANDOM;
      } else {
         // error
         return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[i]);
      }
   }

   size_t list_len = (inputFlag & DO_ARRAY) ? mtest.getArrListSize() : mtest.getObjListSize();
   const string term = (inputFlag & DO_ARRAY) ? TERM_ARRAY : TERM_OBJECT;

   if (inputFlag & DO_INDEX) {
      // always use the correct data type
      size_t objIdx = (size_t)objId;
      // check for id
      if (objIdx >= list_len) {
         cerr << "Size of " << term << " list (" << list_len << ") is <= " << objId << "!!" << endl;
         return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[optLen]);
      }

      if (inputFlag & DO_ARRAY) {
         mtest.deleteArr(objIdx);
      } else {
         mtest.deleteObj(objIdx);
      }
   } else if (inputFlag & DO_RANDOM) {
      if (!list_len) {
         cerr << "Size of " << term << " list is 0!!" << endl;
         // if empty, the illegal part is `-random` instead of the number
         return CmdExec::errorOption(CMD_OPT_ILLEGAL, tokens[optRand - 1]);
      }
      // repeat
      if (inputFlag & DO_ARRAY) {
         for (int i = 0; i < numRandId; i++)
            mtest.deleteArr((size_t)rnGen(list_len));
      } else {
         for (int i = 0; i < numRandId; i++)
            mtest.deleteObj((size_t)rnGen(list_len));
      }
   } else {
      // neither option is specified
      return CmdExec::errorOption(CMD_OPT_MISSING, "");
   }

   // cout << "options: objId=" << objId << ", numRandId=" << numRandId << ". " << endl;

   return CMD_EXEC_DONE;
}

void
MTDeleteCmd::usage(ostream& os) const
{
   os << "Usage: MTDelete <-Index (size_t objId) | "
      << "-Random (size_t numRandId)> [-Array]" << endl;
}

void
MTDeleteCmd::help() const
{
   cout << setw(15) << left << "MTDelete: "
        << "(memory test) delete objects" << endl;
}


//----------------------------------------------------------------------
//    MTPrint
//----------------------------------------------------------------------
CmdExecStatus
MTPrintCmd::exec(const string& option)
{
   // check option
   if (option.size())
      return CmdExec::errorOption(CMD_OPT_EXTRA, option);
   mtest.print();

   return CMD_EXEC_DONE;
}

void
MTPrintCmd::usage(ostream& os) const
{
   os << "Usage: MTPrint" << endl;
}

void
MTPrintCmd::help() const
{
   cout << setw(15) << left << "MTPrint: "
        << "(memory test) print memory manager info" << endl;
}


