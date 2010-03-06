/* ----------------------------------------------------------------------------- 
 * This file is part of SWIG, which is licensed as a whole under version 3 
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * DoxygenEntity.cpp
 *
 * Part of the Doxygen comment translation module of SWIG.
 * ----------------------------------------------------------------------------- */

#include "DoxygenEntity.h"
#include <iostream>

DoxygenEntity::DoxygenEntity(std::string typeEnt) {
  typeOfEntity = typeEnt;
  data = "";
  isLeaf = true;
}

/* Basic node for commands that have
 * only 1 thing after them
 * example: \b word
 * OR holding a std::string
 */
DoxygenEntity::DoxygenEntity(std::string typeEnt, std::string param1) {
  typeOfEntity = typeEnt;
  data = param1;
  isLeaf = true;
}

/* Nonterminal node
 * contains
 */
DoxygenEntity::DoxygenEntity(std::string typeEnt, std::list < DoxygenEntity > &entList) {
  typeOfEntity = typeEnt;
  data = "";
  isLeaf = false;
  entityList = entList;
}

void DoxygenEntity::printEntity(int level) {
  int thisLevel = level;
  if (isLeaf) {
    for (int i = 0; i < thisLevel; i++) {
      std::cout << "\t";
    }

    std::cout << "Node Command: " << typeOfEntity << " ";

    if (data.compare("") != 0) {
      std::cout << "Node Data: " << data;
    }
    std::cout << std::endl;

  } else {

    for (int i = 0; i < thisLevel; i++) {
      std::cout << "\t";
    }

    std::cout << "Node Command : " << typeOfEntity << std::endl;

    std::list < DoxygenEntity >::iterator p = entityList.begin();
    thisLevel++;

    while (p != entityList.end()) {
      (*p).printEntity(thisLevel);
      p++;
    }
  }
}

// not used, completely wrong - currently std lib reports 'invalid operator <'
bool CompareDoxygenEntities::operator() (DoxygenEntity & first, DoxygenEntity & second) {

  // return first.typeOfEntity < second.typeOfEntity;
  if (first.typeOfEntity.compare("brief") == 0)
    return true;
  if (second.typeOfEntity.compare("brief") == 0)
    return false;
  if (first.typeOfEntity.compare("details") == 0)
    return true;
  if (second.typeOfEntity.compare("details") == 0)
    return false;
  if (first.typeOfEntity.compare("partofdescription") == 0)
    return true;
  if (second.typeOfEntity.compare("partofdescription") == 0)
    return false;
  if (first.typeOfEntity.compare("plainstd::string") == 0)
    return true;
  if (second.typeOfEntity.compare("plainstd::string") == 0)
    return false;
  if (first.typeOfEntity.compare("param") == 0) {
    if (second.typeOfEntity.compare("param") == 0)
      return true;
    if (second.typeOfEntity.compare("return") == 0)
      return true;
    if (second.typeOfEntity.compare("exception") == 0)
      return true;
    if (second.typeOfEntity.compare("author") == 0)
      return true;
    if (second.typeOfEntity.compare("version") == 0)
      return true;
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("return") == 0) {
    if (second.typeOfEntity.compare("return") == 0)
      return true;
    if (second.typeOfEntity.compare("exception") == 0)
      return true;
    if (second.typeOfEntity.compare("author") == 0)
      return true;
    if (second.typeOfEntity.compare("version") == 0)
      return true;
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("exception") == 0) {
    if (second.typeOfEntity.compare("exception") == 0)
      return true;
    if (second.typeOfEntity.compare("author") == 0)
      return true;
    if (second.typeOfEntity.compare("version") == 0)
      return true;
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("author") == 0) {
    if (second.typeOfEntity.compare("author") == 0)
      return true;
    if (second.typeOfEntity.compare("version") == 0)
      return true;
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("version") == 0) {
    if (second.typeOfEntity.compare("version") == 0)
      return true;
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("see") == 0 || first.typeOfEntity.compare("sa") == 0) {
    if (second.typeOfEntity.compare("see") == 0)
      return true;
    if (second.typeOfEntity.compare("sa") == 0)
      return true;
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("since") == 0) {
    if (second.typeOfEntity.compare("since") == 0)
      return true;
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  if (first.typeOfEntity.compare("deprecated") == 0) {
    if (second.typeOfEntity.compare("deprecated") == 0)
      return true;
    return false;
  }
  return true;
}
