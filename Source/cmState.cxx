/*============================================================================
  CMake - Cross Platform Makefile Generator
  Copyright 2015 Stephen Kelly <steveire@gmail.com>

  Distributed under the OSI-approved BSD License (the "License");
  see accompanying file Copyright.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the License for more information.
============================================================================*/
#include "cmState.h"

#include "cmake.h"
#include "cmCacheManager.h"
#include "cmCommand.h"
#include "cmAlgorithms.h"

#include <assert.h>

cmState::cmState(cmake* cm)
  : CMakeInstance(cm),
    IsInTryCompile(false)
{
}

cmState::~cmState()
{
  cmDeleteAll(this->Commands);
}

const char* cmCacheEntryTypes[] =
{ "BOOL",
  "PATH",
  "FILEPATH",
  "STRING",
  "INTERNAL",
  "STATIC",
  "UNINITIALIZED",
  0
};

const char*
cmState::CacheEntryTypeToString(cmState::CacheEntryType type)
{
  if ( type > 6 )
    {
    return cmCacheEntryTypes[6];
    }
  return cmCacheEntryTypes[type];
}

cmState::CacheEntryType
cmState::StringToCacheEntryType(const char* s)
{
  int i = 0;
  while(cmCacheEntryTypes[i])
    {
    if(strcmp(s, cmCacheEntryTypes[i]) == 0)
      {
      return static_cast<cmState::CacheEntryType>(i);
      }
    ++i;
    }
  return STRING;
}

bool cmState::IsCacheEntryType(std::string const& key)
{
  for(int i=0; cmCacheEntryTypes[i]; ++i)
    {
    if(strcmp(key.c_str(), cmCacheEntryTypes[i]) == 0)
      {
      return true;
      }
    }
  return false;
}

std::vector<std::string> cmState::GetCacheEntryKeys() const
{
  std::vector<std::string> definitions;
  definitions.reserve(this->CMakeInstance->GetCacheManager()->GetSize());
  cmCacheManager::CacheIterator cit =
    this->CMakeInstance->GetCacheManager()->GetCacheIterator();
  for ( cit.Begin(); !cit.IsAtEnd(); cit.Next() )
    {
    definitions.push_back(cit.GetName());
    }
  return definitions;
}

const char* cmState::GetCacheEntryValue(std::string const& key) const
{
  cmCacheManager::CacheEntry* e = this->CMakeInstance->GetCacheManager()
             ->GetCacheEntry(key);
  if (!e)
    {
    return 0;
    }
  return e->Value.c_str();
}

const char*
cmState::GetInitializedCacheValue(std::string const& key) const
{
  return this->CMakeInstance->GetCacheManager()->GetInitializedCacheValue(key);
}

cmState::CacheEntryType
cmState::GetCacheEntryType(std::string const& key) const
{
  cmCacheManager::CacheIterator it =
      this->CMakeInstance->GetCacheManager()->GetCacheIterator(key.c_str());
  return it.GetType();
}

void cmState::SetCacheEntryValue(std::string const& key,
                                         std::string const& value)
{
  this->CMakeInstance->GetCacheManager()->SetCacheEntryValue(key, value);
}

void cmState::SetCacheEntryProperty(std::string const& key,
                            std::string const& propertyName,
                            std::string const& value)
{
  cmCacheManager::CacheIterator it =
      this->CMakeInstance->GetCacheManager()->GetCacheIterator(key.c_str());
  it.SetProperty(propertyName, value.c_str());
}

void cmState::SetCacheEntryBoolProperty(std::string const& key,
                            std::string const& propertyName,
                            bool value)
{
  cmCacheManager::CacheIterator it =
      this->CMakeInstance->GetCacheManager()->GetCacheIterator(key.c_str());
  it.SetProperty(propertyName, value);
}

const char* cmState::GetCacheEntryProperty(std::string const& key,
                                              std::string const& propertyName)
{
  cmCacheManager::CacheIterator it = this->CMakeInstance->GetCacheManager()
             ->GetCacheIterator(key.c_str());
  if (!it.PropertyExists(propertyName))
    {
    return 0;
    }
  return it.GetProperty(propertyName);
}

bool cmState::GetCacheEntryPropertyAsBool(std::string const& key,
                                              std::string const& propertyName)
{
  return this->CMakeInstance->GetCacheManager()
             ->GetCacheIterator(key.c_str()).GetPropertyAsBool(propertyName);
}

void cmState::AddCacheEntry(const std::string& key, const char* value,
                                    const char* helpString,
                                    cmState::CacheEntryType type)
{
  this->CMakeInstance->GetCacheManager()->AddCacheEntry(key, value,
                                                        helpString, type);
}

void cmState::RemoveCacheEntry(std::string const& key)
{
  this->CMakeInstance->GetCacheManager()->RemoveCacheEntry(key);
}

void cmState::AppendCacheEntryProperty(const std::string& key,
                                               const std::string& property,
                                               const std::string& value,
                                               bool asString)
{
  this->CMakeInstance->GetCacheManager()
       ->GetCacheIterator(key.c_str()).AppendProperty(property,
                                                       value.c_str(),
                                                       asString);
}

void cmState::RemoveCacheEntryProperty(std::string const& key,
                                              std::string const& propertyName)
{
  this->CMakeInstance->GetCacheManager()
       ->GetCacheIterator(key.c_str()).SetProperty(propertyName, (void*)0);
}

void cmState::Initialize()
{
  this->GlobalProperties.clear();

  this->PropertyDefinitions.clear();
  this->DefineProperty
    ("RULE_LAUNCH_COMPILE", cmProperty::DIRECTORY,
     "", "", true);
  this->DefineProperty
    ("RULE_LAUNCH_LINK", cmProperty::DIRECTORY,
     "", "", true);
  this->DefineProperty
    ("RULE_LAUNCH_CUSTOM", cmProperty::DIRECTORY,
     "", "", true);

  this->DefineProperty
    ("RULE_LAUNCH_COMPILE", cmProperty::TARGET,
     "", "", true);
  this->DefineProperty
    ("RULE_LAUNCH_LINK", cmProperty::TARGET,
     "", "", true);
  this->DefineProperty
    ("RULE_LAUNCH_CUSTOM", cmProperty::TARGET,
     "", "", true);
}

void cmState::DefineProperty(const std::string& name,
                           cmProperty::ScopeType scope,
                           const char *ShortDescription,
                           const char *FullDescription,
                           bool chained)
{
  this->PropertyDefinitions[scope].DefineProperty(name,scope,ShortDescription,
                                                  FullDescription,
                                                  chained);
}

cmPropertyDefinition *cmState
::GetPropertyDefinition(const std::string& name,
                        cmProperty::ScopeType scope)
{
  if (this->IsPropertyDefined(name,scope))
    {
    return &(this->PropertyDefinitions[scope][name]);
    }
  return 0;
}

bool cmState::IsPropertyDefined(const std::string& name,
                              cmProperty::ScopeType scope)
{
  return this->PropertyDefinitions[scope].IsPropertyDefined(name);
}

bool cmState::IsPropertyChained(const std::string& name,
                              cmProperty::ScopeType scope)
{
  return this->PropertyDefinitions[scope].IsPropertyChained(name);
}

void cmState::SetLanguageEnabled(std::string const& l)
{
  std::vector<std::string>::iterator it =
      std::lower_bound(this->EnabledLanguages.begin(),
                       this->EnabledLanguages.end(), l);
  if (it == this->EnabledLanguages.end() || *it != l)
    {
    this->EnabledLanguages.insert(it, l);
    }
}

bool cmState::GetLanguageEnabled(std::string const& l) const
{
  return std::binary_search(this->EnabledLanguages.begin(),
                            this->EnabledLanguages.end(), l);
}

std::vector<std::string> cmState::GetEnabledLanguages() const
{
  return this->EnabledLanguages;
}

void cmState::ClearEnabledLanguages()
{
  this->EnabledLanguages.clear();
}

bool cmState::GetIsInTryCompile() const
{
  return this->IsInTryCompile;
}

void cmState::SetIsInTryCompile(bool b)
{
  this->IsInTryCompile = b;
}

void cmState::RenameCommand(std::string const& oldName,
                            std::string const& newName)
{
  // if the command already exists, free the old one
  std::string sOldName = cmSystemTools::LowerCase(oldName);
  std::string sNewName = cmSystemTools::LowerCase(newName);
  std::map<std::string, cmCommand*>::iterator pos =
      this->Commands.find(sOldName);
  if ( pos == this->Commands.end() )
    {
    return;
    }
  cmCommand* cmd = pos->second;

  pos = this->Commands.find(sNewName);
  if (pos != this->Commands.end())
    {
    delete pos->second;
    this->Commands.erase(pos);
    }
  this->Commands.insert(std::make_pair(sNewName, cmd));
  pos = this->Commands.find(sOldName);
  this->Commands.erase(pos);
}

void cmState::AddCommand(cmCommand* command)
{
  std::string name = cmSystemTools::LowerCase(command->GetName());
  // if the command already exists, free the old one
  std::map<std::string, cmCommand*>::iterator pos = this->Commands.find(name);
  if (pos != this->Commands.end())
    {
    delete pos->second;
    this->Commands.erase(pos);
    }
  this->Commands.insert(std::make_pair(name, command));
}

void cmState::RemoveUnscriptableCommands()
{
  std::vector<std::string> unscriptableCommands;
  for (std::map<std::string, cmCommand*>::iterator
       pos = this->Commands.begin();
       pos != this->Commands.end(); )
    {
    if (!pos->second->IsScriptable())
      {
      delete pos->second;
      this->Commands.erase(pos++);
      }
    else
      {
      ++pos;
      }
    }
}

cmCommand* cmState::GetCommand(std::string const& name) const
{
  cmCommand* command = 0;
  std::string sName = cmSystemTools::LowerCase(name);
  std::map<std::string, cmCommand*>::const_iterator pos =
      this->Commands.find(sName);
  if (pos != this->Commands.end())
    {
    command = (*pos).second;
    }
  return command;
}

std::vector<std::string> cmState::GetCommandNames() const
{
  std::vector<std::string> commandNames;
  commandNames.reserve(this->Commands.size());
  std::map<std::string, cmCommand*>::const_iterator cmds
      = this->Commands.begin();
  for ( ; cmds != this->Commands.end(); ++ cmds )
    {
    commandNames.push_back(cmds->first);
    }
  return commandNames;
}

void cmState::RemoveUserDefinedCommands()
{
  for(std::map<std::string, cmCommand*>::iterator j = this->Commands.begin();
      j != this->Commands.end(); )
    {
    if (j->second->IsA("cmMacroHelperCommand") ||
        j->second->IsA("cmFunctionHelperCommand"))
      {
      delete j->second;
      this->Commands.erase(j++);
      }
    else
      {
      ++j;
      }
    }
}

void cmState::SetGlobalProperty(const std::string& prop, const char* value)
{
  this->GlobalProperties.SetProperty(prop, value, cmProperty::GLOBAL);
}

void cmState::AppendGlobalProperty(const std::string& prop,
                                   const char* value, bool asString)
{
  this->GlobalProperties.AppendProperty(prop, value,
                                        cmProperty::GLOBAL, asString);
}

const char *cmState::GetGlobalProperty(const std::string& prop)
{
  // watch for special properties
  std::string output = "";
  if ( prop == "CACHE_VARIABLES" )
    {
    std::vector<std::string> cacheKeys = this->GetCacheEntryKeys();
    this->SetGlobalProperty("CACHE_VARIABLES", cmJoin(cacheKeys, ";").c_str());
    }
  else if ( prop == "COMMANDS" )
    {
    std::vector<std::string> commands = this->GetCommandNames();
    this->SetGlobalProperty("COMMANDS", cmJoin(commands, ";").c_str());
    }
  else if ( prop == "IN_TRY_COMPILE" )
    {
    this->SetGlobalProperty("IN_TRY_COMPILE",
                      this->IsInTryCompile ? "1" : "0");
    }
  else if ( prop == "ENABLED_LANGUAGES" )
    {
    std::string langs;
    langs = cmJoin(this->EnabledLanguages, ";");
    this->SetGlobalProperty("ENABLED_LANGUAGES", langs.c_str());
    }
#define STRING_LIST_ELEMENT(F) ";" #F
  if (prop == "CMAKE_C_KNOWN_FEATURES")
    {
    return FOR_EACH_C_FEATURE(STRING_LIST_ELEMENT) + 1;
    }
  if (prop == "CMAKE_CXX_KNOWN_FEATURES")
    {
    return FOR_EACH_CXX_FEATURE(STRING_LIST_ELEMENT) + 1;
    }
#undef STRING_LIST_ELEMENT
  bool dummy = false;
  return this->GlobalProperties.GetPropertyValue(prop, cmProperty::GLOBAL,
                                                 dummy);
}

bool cmState::GetGlobalPropertyAsBool(const std::string& prop)
{
  return cmSystemTools::IsOn(this->GetGlobalProperty(prop));
}
