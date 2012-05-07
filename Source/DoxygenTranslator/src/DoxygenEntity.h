/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3 
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * DoxygenEntity.h
 *
 * Part of the Doxygen comment translation module of SWIG.
 * ----------------------------------------------------------------------------- */

#ifndef DOXYGENENTITY_H_
#define DOXYGENENTITY_H_

#include <string>
#include <list>


typedef enum {
  SIMPLECOMMAND,
  IGNOREDSIMPLECOMMAND,
  COMMANDWORD,
  IGNOREDCOMMANDWORD,
  COMMANDLINE,
  IGNOREDCOMMANDLINE,
  COMMANDPARAGRAPH,
  IGNORECOMMANDPARAGRAPH,
  COMMANDENDCOMMAND,
  COMMANDWORDPARAGRAPH,
  COMMANDWORDLINE,
  COMMANDWORDOWORDWORD,
  COMMANDOWORD,
  COMMANDERRORTHROW,
  COMMANDUNIQUE,
  END_LINE,
  PARAGRAPH_END,
  PLAINSTRING,
  COMMAND
} DoxyCommandEnum;

/*
 * Structure to represent a doxygen comment entry
 */
struct DoxygenEntity {
  std::string typeOfEntity;
  std::list < DoxygenEntity > entityList;
  std::string data;
  bool isLeaf;

  DoxygenEntity(std::string typeEnt);
  DoxygenEntity(std::string typeEnt, std::string param1);
  DoxygenEntity(std::string typeEnt, std::list < DoxygenEntity > &entList);

  void printEntity(int level);
};

/* 
 * Functor that sorts entities by javaDoc standard order for commands.
 * NOTE: will not behave entirely properly until "First level" comments
 * such as brief descriptions are TAGGED as such
 */
struct CompareDoxygenEntities {
  bool operator() (DoxygenEntity & first, DoxygenEntity & second);
};

#endif
