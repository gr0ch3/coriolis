// ****************************************************************************************************
// File: ./Cell.cpp
// Authors: R. Escassut
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
//
// This file is part of Hurricane.
//
// Hurricane is free software: you can redistribute it  and/or  modify it under the  terms  of the  GNU
// Lesser General Public License as published by the Free Software Foundation, either version 3 of  the
// License, or (at your option) any later version.
//
// Hurricane is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without  even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A  PARTICULAR  PURPOSE. See  the  Lesser  GNU
// General Public License for more details.
//
// You should have received a copy of the Lesser GNU General Public License along  with  Hurricane.  If
// not, see <http://www.gnu.org/licenses/>.
// ****************************************************************************************************

//#define  TEST_INTRUSIVESET

#include "hurricane/DebugSession.h"
#include "hurricane/Warning.h"
#include "hurricane/SharedName.h"
#include "hurricane/DataBase.h"
#include "hurricane/Cell.h"
#include "hurricane/Library.h"
#include "hurricane/Instance.h"
#include "hurricane/Net.h"
#include "hurricane/Pin.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Vertical.h"
#include "hurricane/Contact.h"
#include "hurricane/Pad.h"
#include "hurricane/Rectilinear.h"
#include "hurricane/Layer.h"
#include "hurricane/Slice.h"
#include "hurricane/Rubber.h"
#include "hurricane/Marker.h"
#include "hurricane/Component.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/Error.h"
#include "hurricane/JsonReader.h"

namespace Hurricane {


// ****************************************************************************************************
// UniquifyRelation implementation
// ****************************************************************************************************

  const Name  Cell::UniquifyRelation::_name = "Cell::UniquifyRelation";


  Cell::UniquifyRelation::UniquifyRelation ( Cell* masterOwner )
    : Relation   (masterOwner)
    , _duplicates(1)
  { }


  Cell::UniquifyRelation* Cell::UniquifyRelation::create ( Cell* masterOwner )
  {
    UniquifyRelation* relation = new UniquifyRelation(masterOwner);

    relation->_postCreate();

    return relation;
  }


  void  Cell::UniquifyRelation::_preDestroy ()
  {
    Relation::_preDestroy();
  }


  Cell::UniquifyRelation* Cell::UniquifyRelation::get ( const Cell* cell )
  {
    if (not cell)
      throw Error( "Can't get Cell::UniquifyRelation : empty cell" );
    Property* property = cell->getProperty( staticGetName() );
    if (property) {
      UniquifyRelation* relation = dynamic_cast<UniquifyRelation*>(property);
      if (not relation )
        throw Error ( "Bad Property type: Must be a UniquifyRelation" );
      return relation;
    }
    return NULL;
  }


  Name    Cell::UniquifyRelation::staticGetName () { return _name; }
  Name    Cell::UniquifyRelation::getName       () const { return _name; }
  string  Cell::UniquifyRelation::_getTypeName  () const { return "Cell::UniquifyRelation"; }


  Name  Cell::UniquifyRelation::getUniqueName ()
  {
    Cell*          owner = dynamic_cast<Cell*>( getMasterOwner() );
    ostringstream  name;
    
    name << getString(owner->getName()) << "_u" << setw(2) << setfill('0') << _duplicates++;

    return Name(name.str());
  }


  string  Cell::UniquifyRelation::getTrunkName ( Name name )
  {
    string trunk  = getString(name);
    size_t suffix = trunk.rfind( "_u" );

    if (suffix != string::npos)
      trunk = trunk.substr( 0, suffix );

    return trunk;
  }


  Record* Cell::UniquifyRelation::_getRecord () const
  {
    Record* record = Relation::_getRecord();
    if (record) {
      record->add( getSlot( "_duplicates", &_duplicates ) );
    }
    return record;
  }


  bool  Cell::UniquifyRelation::hasJson () const
  { return true; }


  void  Cell::UniquifyRelation::toJson ( JsonWriter* w, const DBo* owner ) const
  {
    w->startObject();
    std::string tname = getString( staticGetName() );
    if (getMasterOwner() == owner) {
      jsonWrite( w, "@typename"  , tname                 );
      jsonWrite( w, "_refcount"  , getOwners().getSize() );
      jsonWrite( w, "_duplicates", _duplicates           );
    } else {
      tname.insert( 0, "&" );
      jsonWrite( w, "@typename", tname );

      Cell* masterOwner = dynamic_cast<Cell*>( getMasterOwner() );
      if (masterOwner) {
        jsonWrite( w, "_masterOwner", masterOwner->getHierarchicalName() );
      } else {
        cerr << Error( "UniquifyRelation::toJson(): Master owner is not a Cell (%s)."
                     , getString(owner).c_str()
                     ) << endl;
        jsonWrite( w, "_masterOwner", "" );
      }
    }
    w->endObject();
  }


// ****************************************************************************************************
// UniquifyRelation::JsonProperty implementation
// ****************************************************************************************************


  Initializer<Cell::UniquifyRelation::JsonProperty>  jsonUniquifyRelationInit ( 10 );


  Cell::UniquifyRelation::JsonProperty::JsonProperty ( unsigned long flags )
    : JsonObject(flags)
  {
    add( "_refcount"  , typeid(int64_t)  );
    add( "_duplicates", typeid(int64_t) );
  }


  string  Cell::UniquifyRelation::JsonProperty::getTypeName () const
  { return "Cell::UniquifyRelation"; }


  void  Cell::UniquifyRelation::JsonProperty::initialize ()
  { JsonTypes::registerType( new Cell::UniquifyRelation::JsonProperty (JsonWriter::RegisterMode) ); }


  Cell::UniquifyRelation::JsonProperty* Cell::UniquifyRelation::JsonProperty::clone ( unsigned long flags ) const
  { return new Cell::UniquifyRelation::JsonProperty ( flags ); }


  void Cell::UniquifyRelation::JsonProperty::toData ( JsonStack& stack )
  {
    check( stack, "Cell::UniquifyRelation::JsonProperty::toData" );

    DBo*              dbo        = stack.back_dbo();
    unsigned int      refcount   = get<int64_t>( stack, "_refcount"   );
    unsigned int      duplicates = get<int64_t>( stack, "_duplicates" );
    UniquifyRelation* relation   = NULL;
    Cell*             cell       = dynamic_cast<Cell*>( dbo );

    cdebug_log(19,0) << "topDBo:" << dbo << endl;

    if (cell) {
      relation = UniquifyRelation::get( cell );
      if (not relation) {
        string tag = cell->getHierarchicalName()+"::"+getString(UniquifyRelation::staticGetName());
        relation = dynamic_cast<UniquifyRelation*>( SharedProperty::getOrphaned( tag ) );

        if (not relation) {
          relation = Cell::UniquifyRelation::create( cell );
          SharedProperty::addOrphaned( tag, relation );
        }
        SharedProperty::refOrphaned( tag );
        SharedProperty::countOrphaned( tag, refcount );
        cell->put( relation );
      }
      relation->_setMasterOwner( cell );
      relation->_setDuplicates ( duplicates );
    }
    
    update( stack, relation );
  }


// ****************************************************************************************************
// UniquifyRelation::JsonPropertyRef implementation
// ****************************************************************************************************


  Initializer<Cell::UniquifyRelation::JsonPropertyRef>  jsonUniquifyRelationRefInit ( 10 );


  Cell::UniquifyRelation::JsonPropertyRef::JsonPropertyRef ( unsigned long flags )
    : JsonObject(flags)
  {
    add( "_masterOwner", typeid(string)  );
  }


  string  Cell::UniquifyRelation::JsonPropertyRef::getTypeName () const
  { return "&Cell::UniquifyRelation"; }


  void  Cell::UniquifyRelation::JsonPropertyRef::initialize ()
  { JsonTypes::registerType( new Cell::UniquifyRelation::JsonPropertyRef (JsonWriter::RegisterMode) ); }


  Cell::UniquifyRelation::JsonPropertyRef* Cell::UniquifyRelation::JsonPropertyRef::clone ( unsigned long flags ) const
  { return new Cell::UniquifyRelation::JsonPropertyRef ( flags ); }


  void Cell::UniquifyRelation::JsonPropertyRef::toData ( JsonStack& stack )
  {
    check( stack, "Cell::UniquifyRelation::JsonPropertyRef::toData" );

    DBo*              dbo        = stack.back_dbo();
    string            masterName = get<string>( stack, "_masterOwner" );
    UniquifyRelation* relation   = NULL;
    Cell*             cell       = dynamic_cast<Cell*>( dbo );
    string            tag        = masterName+"::"+getString(UniquifyRelation::staticGetName());

    if (cell) {
      if (not relation) {
        relation = dynamic_cast<UniquifyRelation*>( SharedProperty::getOrphaned( tag ) );
        if (not relation) {
          relation = Cell::UniquifyRelation::create( cell );
          SharedProperty::addOrphaned( tag, relation );
        }
      }

      if (relation) {
        cell->put( relation );
        SharedProperty::refOrphaned( tag );
      }
    }

    update( stack, relation );
  }


// ****************************************************************************************************
// SlavedsRelation implementation
// ****************************************************************************************************

  const Name  Cell::SlavedsRelation::_name = "Cell::SlavedsRelation";


  Cell::SlavedsRelation::SlavedsRelation ( Cell* masterOwner )
    : Relation   (masterOwner)
  { }


  Cell::SlavedsRelation* Cell::SlavedsRelation::create ( Cell* masterOwner )
  {
    SlavedsRelation* relation = new SlavedsRelation(masterOwner);

    relation->_postCreate();

    return relation;
  }


  void  Cell::SlavedsRelation::_preDestroy ()
  {
    Relation::_preDestroy();
  }


  Cell::SlavedsRelation* Cell::SlavedsRelation::get ( const Cell* cell )
  {
    if (not cell)
      throw Error( "Can't get Cell::SlavedsRelation : empty cell" );
    Property* property = cell->getProperty( staticGetName() );
    if (property) {
      SlavedsRelation* relation = dynamic_cast<SlavedsRelation*>(property);
      if (not relation )
        throw Error ( "Bad Property type: Must be a SlavedsRelation" );
      return relation;
    }
    return NULL;
  }


  Name    Cell::SlavedsRelation::staticGetName () { return _name; }
  Name    Cell::SlavedsRelation::getName       () const { return _name; }
  string  Cell::SlavedsRelation::_getTypeName  () const { return "Cell::SlavedsRelation"; }


  Record* Cell::SlavedsRelation::_getRecord () const
  {
    Record* record = Relation::_getRecord();
    return record;
  }


  bool  Cell::SlavedsRelation::hasJson () const
  { return true; }


  void  Cell::SlavedsRelation::toJson ( JsonWriter* w, const DBo* owner ) const
  {
    w->startObject();
    std::string tname = getString( staticGetName() );
    if (getMasterOwner() == owner) {
      jsonWrite( w, "@typename"  , tname                 );
      jsonWrite( w, "_refcount"  , getOwners().getSize() );
    } else {
      tname.insert( 0, "&" );
      jsonWrite( w, "@typename", tname );

      Cell* masterOwner = dynamic_cast<Cell*>( getMasterOwner() );
      if (masterOwner) {
        jsonWrite( w, "_masterOwner", masterOwner->getHierarchicalName() );
      } else {
        cerr << Error( "SlavedsRelation::toJson(): Master owner is not a Cell (%s)."
                     , getString(owner).c_str()
                     ) << endl;
        jsonWrite( w, "_masterOwner", "" );
      }
    }
    w->endObject();
  }


// ****************************************************************************************************
// SlavedsRelation::JsonProperty implementation
// ****************************************************************************************************


  Initializer<Cell::SlavedsRelation::JsonProperty>  jsonSlavedsRelationInit ( 10 );


  Cell::SlavedsRelation::JsonProperty::JsonProperty ( unsigned long flags )
    : JsonObject(flags)
  {
    add( "_refcount"  , typeid(int64_t)  );
  }


  string  Cell::SlavedsRelation::JsonProperty::getTypeName () const
  { return "Cell::SlavedsRelation"; }


  void  Cell::SlavedsRelation::JsonProperty::initialize ()
  {
    JsonTypes::registerType( new Cell::SlavedsRelation::JsonProperty (JsonWriter::RegisterMode) );
  }


  Cell::SlavedsRelation::JsonProperty* Cell::SlavedsRelation::JsonProperty::clone ( unsigned long flags ) const
  { return new Cell::SlavedsRelation::JsonProperty ( flags ); }


  void Cell::SlavedsRelation::JsonProperty::toData ( JsonStack& stack )
  {
    check( stack, "Cell::SlavedsRelation::JsonProperty::toData" );

    DBo*             dbo      = stack.back_dbo();
    unsigned int     refcount = get<int64_t>( stack, "_refcount"   );
    SlavedsRelation* relation = NULL;
    Cell*            cell     = dynamic_cast<Cell*>( dbo );

    cdebug_log(19,0) << "topDBo:" << dbo << endl;

    if (cell) {
      relation = SlavedsRelation::get( cell );
      if (not relation) {
        string tag = cell->getHierarchicalName()+"::"+getString(SlavedsRelation::staticGetName());
        relation = dynamic_cast<SlavedsRelation*>( SharedProperty::getOrphaned( tag ) );

        if (not relation) {
          relation = Cell::SlavedsRelation::create( cell );
          SharedProperty::addOrphaned( tag, relation );
        }
        SharedProperty::refOrphaned( tag );
        SharedProperty::countOrphaned( tag, refcount );
        cell->put( relation );
      }
      relation->_setMasterOwner( cell );
    }
    
    update( stack, relation );
  }


// ****************************************************************************************************
// SlavedsRelation::JsonPropertyRef implementation
// ****************************************************************************************************


  Initializer<Cell::SlavedsRelation::JsonPropertyRef>  jsonSlavedsRelationRefInit ( 10 );


  Cell::SlavedsRelation::JsonPropertyRef::JsonPropertyRef ( unsigned long flags )
    : JsonObject(flags)
  {
    add( "_masterOwner", typeid(string)  );
  }


  string  Cell::SlavedsRelation::JsonPropertyRef::getTypeName () const
  { return "&Cell::SlavedsRelation"; }


  void  Cell::SlavedsRelation::JsonPropertyRef::initialize ()
  { JsonTypes::registerType( new Cell::SlavedsRelation::JsonPropertyRef (JsonWriter::RegisterMode) ); }


  Cell::SlavedsRelation::JsonPropertyRef* Cell::SlavedsRelation::JsonPropertyRef::clone ( unsigned long flags ) const
  { return new Cell::SlavedsRelation::JsonPropertyRef ( flags ); }


  void Cell::SlavedsRelation::JsonPropertyRef::toData ( JsonStack& stack )
  {
    check( stack, "Cell::SlavedsRelation::JsonPropertyRef::toData" );

    DBo*             dbo        = stack.back_dbo();
    string           masterName = get<string>( stack, "_masterOwner" );
    SlavedsRelation* relation   = NULL;
    Cell*            cell       = dynamic_cast<Cell*>( dbo );
    string           tag        = masterName+"::"+getString(SlavedsRelation::staticGetName());

    if (cell) {
      if (not relation) {
        relation = dynamic_cast<SlavedsRelation*>( SharedProperty::getOrphaned( tag ) );
        if (not relation) {
          relation = Cell::SlavedsRelation::create( cell );
          SharedProperty::addOrphaned( tag, relation );
        }
      }

      if (relation) {
        cell->put( relation );
        SharedProperty::refOrphaned( tag );
      }
    }

    update( stack, relation );
  }


// ****************************************************************************************************
// Cell Slice related implementation
// ****************************************************************************************************

  void  Cell::_insertSlice ( ExtensionSlice* slice )
  {
    ExtensionSliceMap::iterator islice = _extensionSlices.find ( slice->getName() );
    if ( islice != _extensionSlices.end() )
      throw Error ( "Attempt to re-create ExtensionSlice %s in Cell %s."
                  , getString(slice->getName()).c_str()
                  , getString(slice->getCell()->getName()).c_str()
                  );

    _extensionSlices.insert ( pair<Name,ExtensionSlice*>(slice->getName(),slice) );
  }


  void  Cell::_removeSlice ( ExtensionSlice* slice )
  {
    ExtensionSliceMap::iterator islice = _extensionSlices.find ( slice->getName() );
    if ( islice != _extensionSlices.end() ) {
      _extensionSlices.erase ( islice );
    }
  }


  ExtensionSlice* Cell::getExtensionSlice ( const Name& name ) const
  {
    ExtensionSliceMap::const_iterator islice = _extensionSlices.find ( name );
    if ( islice != _extensionSlices.end() )
      return islice->second;

    return NULL;
  }


  ExtensionSlice::Mask  Cell::getExtensionSliceMask ( const Name& name ) const
  {
    ExtensionSliceMap::const_iterator islice = _extensionSlices.find ( name );
    if ( islice != _extensionSlices.end() )
      return islice->second->getMask();

    return 0;
  }


// ****************************************************************************************************
// Cell implementation
// ****************************************************************************************************


Cell::Cell(Library* library, const Name& name)
// *******************************************
:    Inherit(),
    _library(library),
    _name(name),
    _shuntedPath(),
    _instanceMap(),
     _quadTree(new QuadTree()),
    _slaveInstanceSet(),
    _netMap(),
     _sliceMap(new SliceMap()),
    _extensionSlices(),
    _markerSet(),
    //_viewSet(),
    _abutmentBox(),
    _boundingBox(),
    _nextOfLibraryCellMap(NULL),
    _nextOfSymbolCellSet(NULL),
    _slaveEntityMap(),
    _observers(),
    _flags(Flags::NoFlags)
{
  if (!_library)
    throw Error("Can't create " + _TName("Cell") + " : null library");

  if (name.isEmpty())
    throw Error("Can't create " + _TName("Cell") + " : empty name");

  if (_library->getCell(_name))
    throw Error("Can't create " + _TName("Cell") + " " + getString(_name) + " : already exists");
}

Cell* Cell::create(Library* library, const Name& name)
// ***************************************************
{
    Cell* cell = new Cell(library, name);

    cell->_postCreate();

    return cell;
}

Cell* Cell::fromJson(const string& filename)
// *****************************************
{
  UpdateSession::open();

  JsonReader reader ( JsonWriter::CellMode );
  reader.parse( filename );

  UpdateSession::close();

  const JsonStack& stack = reader.getStack();
  if (stack.rhas(".Cell")) return stack.as<Cell*>(".Cell");

  return NULL;
}

Box Cell::getBoundingBox() const
// *****************************
{
    if (_boundingBox.isEmpty()) {
        Box& boundingBox = (Box&)_boundingBox;
        boundingBox = _abutmentBox;
        boundingBox.merge(_quadTree->getBoundingBox());
        for_each_slice(slice, getSlices()) {
            boundingBox.merge(slice->getBoundingBox());
            end_for;
        }
    }
    
    return _boundingBox;
}

bool Cell::isCalledBy ( Cell* cell ) const
{
  for ( Instance* instance : cell->getInstances() ) {
    Cell* masterCell = instance->getMasterCell();
    if (masterCell == this) return true;
    if (isCalledBy(masterCell)) return true;
  }
  return false;
}

bool Cell::isNetAlias ( const Name& name ) const
// *********************************************
{
  NetAliasName key(name);
  return _netAliasSet.find(&key) != _netAliasSet.end();
}

bool Cell::isUnique() const
// ************************
{
  // ltrace(10) << "Cell::isUnique() " << this << endl;
  // for ( Instance* instance : getSlaveInstances() ) {
  //   ltrace(10) << "| Slave instance:" << instance << endl;
  // }
  return getSlaveInstances().getSize() < 2;
}

bool Cell::isUniquified() const
// ****************************
{
  UniquifyRelation* relation = UniquifyRelation::get( this );
  return relation and (relation->getMasterOwner() != this);
}

bool Cell::isUniquifyMaster() const
// ********************************
{
  UniquifyRelation* relation = UniquifyRelation::get( this );
  return (not relation) or (relation->getMasterOwner() == this);
}

string Cell::getHierarchicalName () const
// **************************************
{
  return getLibrary()->getHierarchicalName() + "." + getString(getName());
}

Entity* Cell::getEntity(const Signature& signature) const
// ******************************************************
{
  if (  (signature.getType() == Signature::TypeContact   )
     or (signature.getType() == Signature::TypeHorizontal)
     or (signature.getType() == Signature::TypeVertical  )
     or (signature.getType() == Signature::TypePad       ) ) {
    Net* net = getNet( signature.getName() );
    if (not net) {
      cerr << Error( "Cell::getEntity(): Cell %s do have Net %s, signature incoherency."
                   , getString(getName()).c_str()
                   , signature.getName().c_str() ) << endl;
      return NULL;
    }

    cdebug_log(18,0) << "Cell::getEntity(): <" << getName() << ">, Net:<" << net->getName() << ">" << endl;

    if (signature.getType() == Signature::TypeContact) {
      cdebug_log(18,0) << "Looking in Contacts..." << endl;
      for ( Contact* component : getComponents().getSubSet<Contact*>() ) {
        cdebug_log(18,0) << "| " << component << endl;
        if (   (component->getLayer () == signature.getLayer())
           and (component->getDx    () == signature.getDim(Signature::ContactDx))
           and (component->getDy    () == signature.getDim(Signature::ContactDy))
           and (component->getWidth () == signature.getDim(Signature::ContactWidth))
           and (component->getHeight() == signature.getDim(Signature::ContactHeight)) )
          return component;
      }
    }

    if (signature.getType() == Signature::TypeVertical) {
      cdebug_log(18,0) << "Looking in Verticals..." << endl;
      for ( Vertical* component : getComponents().getSubSet<Vertical*>() ) {
        cdebug_log(18,0) << "| " << component << endl;
        if (   (component->getLayer   () == signature.getLayer())
           and (component->getWidth   () == signature.getDim(Signature::VerticalWidth))
           and (component->getX       () == signature.getDim(Signature::VerticalX))
           and (component->getDySource() == signature.getDim(Signature::VerticalDySource))
           and (component->getDyTarget() == signature.getDim(Signature::VerticalDyTarget)) )
          return component;
      }
    }

    if (signature.getType() == Signature::TypeHorizontal) {
      cdebug_log(18,0) << "Looking in Horizontals..." << endl;
      for ( Horizontal* component : getComponents().getSubSet<Horizontal*>() ) {
        cdebug_log(18,0) << "| " << component << endl;
        if (   (component->getLayer   () == signature.getLayer())
           and (component->getWidth   () == signature.getDim(Signature::HorizontalWidth))
           and (component->getY       () == signature.getDim(Signature::HorizontalY))
           and (component->getDxSource() == signature.getDim(Signature::HorizontalDxSource))
           and (component->getDxTarget() == signature.getDim(Signature::HorizontalDxTarget)) )
          return component;
      }
    }

    if (signature.getType() == Signature::TypePad) {
      cdebug_log(18,0) << "Looking in Pads..." << endl;
      for ( Pad* component : getComponents().getSubSet<Pad*>() ) {
        cdebug_log(18,0) << "| " << component << endl;
        if (   (component->getLayer()                 == signature.getLayer())
           and (component->getBoundingBox().getXMin() == signature.getDim(Signature::PadXMin))
           and (component->getBoundingBox().getYMin() == signature.getDim(Signature::PadYMin))
           and (component->getBoundingBox().getXMax() == signature.getDim(Signature::PadXMax))
           and (component->getBoundingBox().getYMax() == signature.getDim(Signature::PadYMax)) )
          return component;
      }
    }

    cerr << Error( "Cell::getEntity(): Cannot find a Component of type %d matching Signature."
                 , signature.getType() ) << endl;
  } else {
    cerr << Error( "Cell::getEntity(): Signature type %d is unsupported yet."
                 , signature.getType() ) << endl;
  }

  return NULL;
}

Net* Cell::getNet ( const Name& name, bool useAlias ) const
//*********************************************************
{
  Net* net = _netMap.getElement(name);
  if (net) return net;

  NetAliasName key(name);
  AliasNameSet::iterator ialias = _netAliasSet.find( &key );
  if (ialias != _netAliasSet.end()) {
    if (not (useAlias or (*ialias)->isExternal()))
      return NULL;
    return (*ialias)->getNet();
  }

  return NULL;
}


void Cell::setName(const Name& name)
// *********************************
{
    if (name != _name) {
        if (name.isEmpty())
            throw Error("Can't change " + _TName("Cell") + " name : empty name");

        if (_library->getCell(name))
            throw Error("Can't change " + _TName("Cell") + " name : already exists");

        _library->_getCellMap()._remove(this);
        _name = name;
        _library->_getCellMap()._insert(this);
    }
}

void Cell::setAbutmentBox(const Box& abutmentBox)
// ***********************************************
{
  SlavedsRelation* slaveds = SlavedsRelation::get( this );
  if (not slaveds or (this == slaveds->getMasterOwner())) {
    _setAbutmentBox( abutmentBox ); 

    if (getFlags().isset(Flags::SlavedAb)) return;

    for ( Cell* slavedCell : SlavedsSet(this) )
      slavedCell->_setAbutmentBox( abutmentBox );
  } else {
    cerr << Error( "Cell::setAbutmentBox(): Abutment box of \"%s\" is slaved to \"%s\"."
                 , getString(getName()).c_str()
                 , getString(static_cast<Cell*>(slaveds->getMasterOwner())->getName()).c_str()
                 ) << endl;
  }
}

void Cell::_setAbutmentBox(const Box& abutmentBox)
// ***********************************************
{
  if (abutmentBox != _abutmentBox) {
    if (not _abutmentBox.isEmpty() and
       (abutmentBox.isEmpty() or not abutmentBox.contains(_abutmentBox)))
      _unfit( _abutmentBox );
    _abutmentBox = abutmentBox;
    _fit( _abutmentBox );
  }
}


DeepNet* Cell::getDeepNet ( Path path, const Net* leafNet ) const
// **************************************************************
{
  if (not (_flags.isset(Flags::FlattenedNets))) return NULL;

  Occurrence rootNetOccurrence ( getHyperNetRootNetOccurrence(Occurrence(leafNet,path)) );
  DeepNet::Uplink* uplink = static_cast<DeepNet::Uplink*>
    ( rootNetOccurrence.getProperty( DeepNet::Uplink::staticGetName() ));
  if (not uplink) return NULL;
  return uplink->getUplink();
}

void Cell::flattenNets (uint64_t flags )
// *************************************
{
  flattenNets( NULL, flags );
}

void Cell::flattenNets ( const Instance* instance, uint64_t flags )
// ****************************************************************
{
  static set<string> excludeds;
  flattenNets( instance, excludeds, flags );
}

void Cell::flattenNets ( const Instance* instance, const std::set<string>& excludeds, uint64_t flags )
// ***************************************************************************************************
{
  cdebug_log(18,1) << "Cell::flattenNets() flags:0x" << hex << flags << endl;

  UpdateSession::open();

  bool reFlatten = _flags.isset(Flags::FlattenedNets);

  _flags |= Flags::FlattenedNets;

  vector<HyperNet>  hyperNets;
  vector<HyperNet>  topHyperNets;

  for ( Occurrence occurrence : getHyperNetRootNetOccurrences().getSubSet(Occurrence_Contains(instance)) ) {
    Net* net = static_cast<Net*>(occurrence.getEntity());

    if (net->isClock() and (flags & Flags::NoClockFlatten)) continue;
    if (net->isPower() or net->isGround() or net->isBlockage()) continue;
    if (excludeds.find(getString(occurrence.getName())) != excludeds.end()) continue;

    HyperNet  hyperNet ( occurrence );
    if ( not occurrence.getPath().isEmpty() ) {
      Net* duplicate = getNet( occurrence.getName() );
      if (not duplicate) {
        hyperNets.push_back( HyperNet(occurrence) );
      } else {
        if (not reFlatten)
          cerr << Warning( "Cell::flattenNets(): In \"%s\", found duplicate: %s for %s."
                         , getString(duplicate->getCell()->getName()).c_str()
                         , getString(duplicate).c_str()
                         , getString(net).c_str()
                         ) << endl;
      }
      continue;
    }

    bool hasRoutingPads = false;
    for ( Component* component : net->getComponents() ) {
      RoutingPad* rp = dynamic_cast<RoutingPad*>( component );
      if (rp) {
      // At least one RoutingPad is present: assumes that the net is already
      // flattened (completly).
        hasRoutingPads = true;
        break;
      }
    }
    if (hasRoutingPads) continue;

    topHyperNets.push_back( HyperNet(occurrence) );
  }

  for ( size_t i=0 ; i<hyperNets.size() ; ++i ) {
    DeepNet* deepNet = DeepNet::create( hyperNets[i] );
    cdebug_log(18,1) << "Flattening hyper net: " << deepNet << endl;
    if (deepNet) deepNet->_createRoutingPads( flags );
    cdebug_log(18,0) << "Done: " << deepNet << endl;
    cdebug_tabw(18,-1);
  }
  cdebug_log(18,0) << "Non-root HyperNet (DeepNet) done" << endl;

  unsigned int rpFlags = (flags & Flags::StayOnPlugs) ? 0 : RoutingPad::BiggestArea;
    
  for ( size_t i=0 ; i<topHyperNets.size() ; ++i ) {
    Net* net = static_cast<Net*>(topHyperNets[i].getNetOccurrence().getEntity());

    DebugSession::open( net, 18, 19 ); 
    cdebug_log(18,1) << "Flattening top net: " << net << endl;

    vector<Occurrence>  plugOccurrences;
    for ( Occurrence plugOccurrence : topHyperNets[i].getTerminalNetlistPlugOccurrences() )
      plugOccurrences.push_back( plugOccurrence );

    for ( Occurrence plugOccurrence : plugOccurrences ) {
      RoutingPad* rp = RoutingPad::create( net, plugOccurrence, rpFlags );
      rp->materialize();

      if (flags & Flags::WarnOnUnplacedInstances)
        rp->isPlacedOccurrence( RoutingPad::ShowWarning );
    }

    cdebug_log(18,0) << "Processing Pins" << endl;
    vector<Pin*> pins;
    for ( Component* component : net->getComponents() ) {
      Pin* pin = dynamic_cast<Pin*>( component );
      if (pin) pins.push_back( pin );
    }
    for ( Pin* pin : pins ) RoutingPad::create( pin );

    cdebug_tabw(18,-1);
    DebugSession::close();
  }

  cdebug_log(18,0) << "Before closing UpdateSession" << endl;
  UpdateSession::close();
  cdebug_log(18,-1) << "Cell::flattenNets() Done" << endl;
}


void Cell::createRoutingPadRings(uint64_t flags)
// *************************************************
{
  flags &= Flags::MaskRings;

  UpdateSession::open();

  for ( Net* net : getNets() ) {
    RoutingPad* previousRp = NULL;
    bool        buildRing  = false;

    if (net->isGlobal()) {
      if      ( (flags & Cell::Flags::BuildClockRings ) and net->isClock () ) buildRing = true;
      else if ( (flags & Cell::Flags::BuildSupplyRings) and net->isSupply() ) buildRing = true;
    } else {
      buildRing = flags & Cell::Flags::BuildRings;
    }
    if (not buildRing) continue;

    for ( Component* component : net->getComponents() ) {
      if (dynamic_cast<Segment*>(component)) { buildRing = false; break; }

      Plug* primaryPlug = dynamic_cast<Plug*>( component );
      if (primaryPlug) {
        if (not primaryPlug->getBodyHook()->getSlaveHooks().isEmpty()) {
          cerr << "[ERROR] " << primaryPlug << "\n"
               << "        has attached components, not managed yet." << endl;
        } else {
          primaryPlug->getBodyHook()->detach();
        }
      }
    }
    if (not buildRing) continue;

    for ( RoutingPad* rp : net->getRoutingPads() ) {
      if ( previousRp
         and (  not rp        ->getBodyHook()->isAttached()
             or not previousRp->getBodyHook()->isAttached()) ) {
        rp->getBodyHook()->attach( previousRp->getBodyHook() );
      }
      previousRp = rp;
    }
  }

  UpdateSession::close();
}


void Cell::destroyPhysical()
// *************************
{
  cdebug_log(18,0) << "Cell::destroyPhysical()" << endl;

  UpdateSession::open();
  for ( Net* net : getNets() ) {
    vector<Component*> removeds;
    for ( Component* component : net->getComponents() ) {
      if (dynamic_cast<RoutingPad*>(component)) removeds.push_back( component );
    }
    for ( Component* component : removeds ) component->destroy();
  }

  vector<DeepNet*> deepNets;
  for ( Net* net : getNets() ) {
    DeepNet* deepNet = dynamic_cast<DeepNet*>( net );
    if (deepNet) deepNets.push_back( deepNet );
  }
  for ( DeepNet* deepNet : deepNets ) {
    cerr << "Removing DeepNet:" << deepNet << endl;
    deepNet->destroy();
  }

  for ( Net* net : getNets() ) {
    vector<Component*> removeds;
    for ( Component* component : net->getComponents() ) {
      if (dynamic_cast<Plug   *>(component)) continue;
      if (dynamic_cast<Contact*>(component)) removeds.push_back( component );
    }
    for ( Component* component : removeds ) component->destroy();
  }

  for ( Net* net : getNets() ) {
    vector<Component*> removeds;
    for ( Component* component : net->getComponents() ) {
      if (dynamic_cast<Plug*>(component)) continue;
      removeds.push_back( component );
    }
    for ( Component* component : removeds ) component->destroy();
  }
  UpdateSession::close();
}

Cell* Cell::getCloneMaster() const
// *******************************
{
  UniquifyRelation* uniquify = UniquifyRelation::get( this );
  if (not uniquify) return const_cast<Cell*>(this);

  return dynamic_cast<Cell*>( uniquify->getMasterOwner() );
}


bool Cell::updatePlacedFlag()
// **************************
{
  bool isPlaced = true;
  for ( Instance* instance : getInstances() ) {
    if (instance->getPlacementStatus() == Instance::PlacementStatus::UNPLACED) {
      isPlaced = false;
      break;
    }
  }
  if (isPlaced) setFlags( Cell::Flags::Placed );
  return isPlaced;
}


Cell* Cell::getClone()
// *******************
{
  UpdateSession::open();

  UniquifyRelation* uniquify = UniquifyRelation::get( this );
  if (not uniquify) {
    uniquify = UniquifyRelation::create( this );
  }

  Cell* clone = Cell::create( getLibrary(), uniquify->getUniqueName() );
  clone->put               ( uniquify );
  clone->setTerminalNetlist( isTerminalNetlist () );
  clone->setPad            ( isPad         () );
  clone->setAbutmentBox    ( getAbutmentBox() );

  for ( Net* inet : getNets() ) {
    if (dynamic_cast<DeepNet*>(inet)) continue;
    inet->getClone( clone );
  }

  bool isPlaced = true;
  for ( Instance* instance : getInstances() ) {
    if (instance->getClone(clone)->getPlacementStatus() == Instance::PlacementStatus::UNPLACED)
      isPlaced = false;
  }
  if (isPlaced) clone->setFlags( Flags::Placed );

  UpdateSession::close();

  return clone;
}

void Cell::uniquify(unsigned int depth)
// ************************************
{
  cdebug_log(18,1) << "Cell::uniquify() " << this << endl;

  vector<DeepNet*>  deepNets;
  for ( DeepNet* deepNet : getNets().getSubSet<DeepNet*>() ) {
    deepNets.push_back( deepNet );
  }
  while ( not deepNets.empty() ) {
    deepNets.back()->destroy();
    deepNets.pop_back();
  }

  vector<Instance*>               toUniquify;
  set<Cell*,Entity::CompareById>  masterCells;

  for ( Instance* instance : getInstances() ) {
    Cell* masterCell = instance->getMasterCell();
    cdebug_log(18,0) << "| " << instance << endl;
    if (masterCell->isTerminal()) continue;

    if (masterCells.find(masterCell) == masterCells.end()) {
      masterCells.insert( masterCell );
      masterCell->updatePlacedFlag();
    }

    if ( (masterCell->getSlaveInstances().getSize() > 1) and not masterCell->isPlaced() ) {
      toUniquify.push_back( instance );
    }
  }

  for ( auto instance : toUniquify ) {
    instance->uniquify();
    masterCells.insert( instance->getMasterCell() );
  }

  if (depth > 0) {
    for ( auto cell : masterCells )
      cell->uniquify( depth-1 );
  }

  cdebug_tabw(18,-1);
  cdebug_log(18,0) << "Cell::uniquify() END " << this << endl;
}

void Cell::materialize()
// *********************
{
  if (_flags.isset(Flags::Materialized)) return;

  cdebug_log(18,1) << "Cell::materialize() " << this << endl;

  _flags |= Flags::Materialized;

  for ( Instance* instance : getInstances() ) {
    if ( instance->getPlacementStatus() != Instance::PlacementStatus::UNPLACED )
      instance->materialize();
  }

  for ( Net*    net    : getNets   () ) net   ->materialize();
  for ( Marker* marker : getMarkers() ) marker->materialize();

  cdebug_tabw(18,-1);
}

void Cell::unmaterialize()
// ***********************
{
  if (not _flags.isset(Flags::Materialized)) return;

  _flags &= ~Flags::Materialized;

  for ( Instance* instance : getInstances()) instance->unmaterialize();
  for ( Net*      net      : getNets()     ) net     ->unmaterialize();
  for ( Marker*   marker   : getMarkers()  ) marker  ->unmaterialize();
}

void Cell::slaveAbutmentBox ( Cell* topCell )
// ******************************************
{
  if (_flags.isset(Flags::SlavedAb)) {
    cerr << Error( "Cell::slaveAbutmentBox(): %s is already slaved, action cancelled."
                 , getString(this).c_str() ) << endl;
    return;
  }

  if (not isUnique()) {
    cerr << Error( "Cell::slaveAbutmentBox(): %s is *not* unique, action cancelled."
                 , getString(this).c_str() ) << endl;
    return;
  }

  _slaveAbutmentBox( topCell );
}

void Cell::_slaveAbutmentBox ( Cell* topCell )
// *******************************************
{
  if (not getAbutmentBox().isEmpty()) {
    if (  (getAbutmentBox().getWidth() != topCell->getAbutmentBox().getWidth())
       or (getAbutmentBox().getWidth() != topCell->getAbutmentBox().getWidth()) ) {
      cerr << Warning( "Slaving abutment boxes of different sizes, fixed blocks may shift.\n"
                       "          topCell: %s (AB:%s)\n"
                       "          slave  : %s (AB:%s)"
                     , getString(topCell->getName()).c_str()
                     , getString(topCell->getAbutmentBox()).c_str()
                     , getString(getName()).c_str()
                     , getString(getAbutmentBox()).c_str()
                     );
    }

    Transformation transf ( topCell->getAbutmentBox().getXMin() - getAbutmentBox().getXMin()
                          , topCell->getAbutmentBox().getYMin() - getAbutmentBox().getYMin() );
    
    for ( Instance* instance : getInstances() ) {
      if (instance->getPlacementStatus() != Instance::PlacementStatus::UNPLACED) {
        Transformation instanceTransf = instance->getTransformation();
        transf.applyOn( instanceTransf );
        instance->setTransformation( instanceTransf );
      }
    }
  }

  _setAbutmentBox( topCell->getAbutmentBox() );

  SlavedsRelation* slaveds = SlavedsRelation::get( topCell );
  if (not slaveds) {
    slaveds = SlavedsRelation::create( topCell );
  }
  put( slaveds );
  _flags.set( Flags::SlavedAb );
}


void Cell::_postCreate()
// *********************
{
    Inherit::_postCreate();

    _library->_getCellMap()._insert(this);
}

void Cell::_preDestroy()
// ********************
{
  notify( Flags::CellDestroyed );

  while ( _slaveEntityMap.size() ) {
    _slaveEntityMap.begin()->second->destroy();
  }

  Markers   markers   = getMarkers       (); while ( markers  .getFirst() ) markers  .getFirst()->destroy();
  Instances instances = getSlaveInstances(); while ( instances.getFirst() ) instances.getFirst()->destroy();

  Nets nets = getNets();
  while ( nets.getFirst() ) {
    Net* net = nets.getFirst();
    net->_getMainName().detachAll();
    net->destroy();
  }
  for ( auto islave : _netAliasSet ) delete islave;

  instances = getInstances();
  vector<Instance*> inss;
  for ( Instance* instance : getInstances() ) inss.push_back( instance );
  for ( Instance* instance : inss ) instance->destroy();
//while ( instances.getFirst() ) instances.getFirst()->destroy();

  for ( Slice* slice  : getSlices()  ) slice->_destroy();
  while ( not _extensionSlices.empty() ) _removeSlice( _extensionSlices.begin()->second );

  delete _sliceMap;
  delete _quadTree;
 
  _library->_getCellMap()._remove( this );

  Inherit::_preDestroy();
}

string Cell::_getString() const
// ****************************
{
    string s = Inherit::_getString();
    s.insert(s.length() - 1, " " + getString(_name));
    return s;
}

Record* Cell::_getRecord() const
// ***********************
{
    Record* record = Inherit::_getRecord();
    if (record) {
        record->add( getSlot("_library"        , _library          ) );
        record->add( getSlot("_name"           , &_name            ) );
        record->add( getSlot("_instances"      , &_instanceMap     ) );
        record->add( getSlot("_quadTree"       ,  _quadTree        ) );
        record->add( getSlot("_extensionSlices", &_extensionSlices ) );
        record->add( getSlot("_slaveInstances" , &_slaveInstanceSet) );
        record->add( getSlot("_netMap"         , &_netMap          ) );
        record->add( getSlot("_netAliasSet"    , &_netAliasSet     ) );
        record->add( getSlot("_pinMap"         , &_pinMap          ) );
        record->add( getSlot("_sliceMap"       ,  _sliceMap        ) );
        record->add( getSlot("_markerSet"      , &_markerSet       ) );
        record->add( getSlot("_slaveEntityMap" , &_slaveEntityMap  ) );
        record->add( getSlot("_abutmentBox"    , &_abutmentBox     ) );
        record->add( getSlot("_boundingBox"    , &_boundingBox     ) );
        record->add( getSlot("_flags"          , &_flags           ) );
    }
    return record;
}

void Cell::_fit(const Box& box)
// ****************************
{
    if (box.isEmpty()) return;
    if (_boundingBox.isEmpty()) return;
    if (_boundingBox.contains(box)) return;
    _boundingBox.merge(box);
    for ( Instance* iinstance : getSlaveInstances() ) {
      iinstance->getCell()->_fit(iinstance->getTransformation().getBox(box));
    }
}

void Cell::_unfit(const Box& box)
// ******************************
{
    if (box.isEmpty()) return;
    if (_boundingBox.isEmpty()) return;
    if (!_boundingBox.isConstrainedBy(box)) return;
    _boundingBox.makeEmpty();
    for ( Instance* iinstance : getSlaveInstances() ) {
        iinstance->getCell()->_unfit(iinstance->getTransformation().getBox(box));
    }
}

void Cell::_addSlaveEntity(Entity* entity, Entity* slaveEntity)
// ************************************************************************
{
  assert(entity->getCell() == this);

  _slaveEntityMap.insert(pair<Entity*,Entity*>(entity,slaveEntity));
}

void Cell::_removeSlaveEntity(Entity* entity, Entity* slaveEntity)
// ***************************************************************************
{
  assert(entity->getCell() == this);

  pair<SlaveEntityMap::iterator,SlaveEntityMap::iterator>
    bounds = _slaveEntityMap.equal_range(entity);
  SlaveEntityMap::iterator it = bounds.first;
  for(; it != bounds.second ; it++ ) {
    if (it->second == slaveEntity) {
      _slaveEntityMap.erase(it);
      break;
    }
  }
}

void Cell::_getSlaveEntities(SlaveEntityMap::iterator& begin, SlaveEntityMap::iterator& end)
// *********************************************************************************************************
{
  begin = _slaveEntityMap.begin();
  end   = _slaveEntityMap.end();
}

void Cell::_getSlaveEntities(Entity* entity, SlaveEntityMap::iterator& begin, SlaveEntityMap::iterator& end)
// *********************************************************************************************************
{
  begin = _slaveEntityMap.lower_bound(entity);
  end   = _slaveEntityMap.upper_bound(entity);
}

void Cell::addObserver(BaseObserver* observer)
// *******************************************
{
  _observers.addObserver(observer);
}

void Cell::removeObserver(BaseObserver* observer)
// **********************************************
{
  _observers.removeObserver(observer);
}

void Cell::notify(unsigned flags)
// ******************************
{
  _observers.notify(flags);
}

void Cell::_toJson(JsonWriter* writer) const
// *****************************************
{
  Inherit::_toJson( writer );

  jsonWrite( writer, "_library"    , getLibrary()->getHierarchicalName() );
  jsonWrite( writer, "_name"       , getName() );
  jsonWrite( writer, "_abutmentBox", &_abutmentBox );
}

void Cell::_toJsonCollections(JsonWriter* writer) const
// *****************************************
{
  writer->setFlags( JsonWriter::CellObject );
  jsonWrite( writer, "+instanceMap", getInstances() );
  jsonWrite( writer, "+netMap"     , getNets() );
  Inherit::_toJsonCollections( writer );
  writer->resetFlags( JsonWriter::CellObject );
}

// ****************************************************************************************************
// Cell::Flags implementation
// ****************************************************************************************************

  Cell::Flags::Flags ( uint64_t flags)
    : BaseFlags(flags)
  { }


  Cell::Flags::~Flags ()
  { }


  string  Cell::Flags::_getTypeName () const
  { return _TName("Cell::Flags"); }


  string Cell::Flags::_getString () const
  {
    if (not _flags) return "<NoFlags>";

    string s = "<";
    if (_flags & Pad             ) { s += "Pad"; }
    if (_flags & TerminalNetlist ) { if (s.size() > 1) s += "|"; s += "TerminalNetlist"; }
    if (_flags & FlattenedNets   ) { if (s.size() > 1) s += "|"; s += "FlattenedNets"; }
    if (_flags & Placed          ) { if (s.size() > 1) s += "|"; s += "Placed"; }
    if (_flags & Routed          ) { if (s.size() > 1) s += "|"; s += "Routed"; }
    if (_flags & AbstractedSupply) { if (s.size() > 1) s += "|"; s += "AbstractedSupply"; }
    if (_flags & SlavedAb        ) { if (s.size() > 1) s += "|"; s += "SlavedAb"; }
    if (_flags & Materialized    ) { if (s.size() > 1) s += "|"; s += "Materialized"; }
    s += ">";

    return s;
  }


// ****************************************************************************************************
// Cell::ClonedSet implementation
// ****************************************************************************************************

  Cell::ClonedSet::Locator::Locator ( const Cell* cell )
    : Hurricane::Locator<Cell*>()
    , _dboLocator              (NULL)
  {
    UniquifyRelation* uniquify = UniquifyRelation::get( cell );
    if (uniquify) {
      _dboLocator = uniquify->getSlaveOwners().getLocator();
    }
  }


  Locator<Cell*>* Cell::ClonedSet::Locator::getClone () const
  { return new Locator(*this); }


  Cell* Cell::ClonedSet::Locator::getElement () const
  { return (_dboLocator and _dboLocator->isValid())
      ? dynamic_cast<Cell*>(_dboLocator->getElement()) : NULL; }


  bool  Cell::ClonedSet::Locator::isValid () const
  { return (_dboLocator and _dboLocator->isValid()); }


  void  Cell::ClonedSet::Locator::progress ()
  {
    _dboLocator->progress();
  }


  string  Cell::ClonedSet::Locator::_getString () const
  {
    string s = "<" + _TName("Cell::ClonedSet::Locator")
                   + getString(getElement())
                   + ">";
    return s;
  }


  Collection<Cell*>* Cell::ClonedSet::getClone   () const
  { return new ClonedSet(*this); }


  Locator<Cell*>* Cell::ClonedSet::getLocator () const
  { return new Locator(_cell); }


  string  Cell::ClonedSet::_getString () const
  {
    string s = "<" + _TName("Cell_ClonedSet") + " "
                   + getString(_cell->getName())
                   + ">";
    return s;
  }


// ****************************************************************************************************
// Cell::SlavedsSet implementation
// ****************************************************************************************************

  Cell::SlavedsSet::Locator::Locator ( const Cell* cell )
    : Hurricane::Locator<Cell*>()
    , _dboLocator              (NULL)
  {
    SlavedsRelation* slaveds = SlavedsRelation::get( cell );
    if (slaveds) {
      _dboLocator = slaveds->getSlaveOwners().getLocator();
    }
  }


  Locator<Cell*>* Cell::SlavedsSet::Locator::getClone () const
  { return new Locator(*this); }


  Cell* Cell::SlavedsSet::Locator::getElement () const
  { return (_dboLocator and _dboLocator->isValid())
      ? dynamic_cast<Cell*>(_dboLocator->getElement()) : NULL; }


  bool  Cell::SlavedsSet::Locator::isValid () const
  { return (_dboLocator and _dboLocator->isValid()); }


  void  Cell::SlavedsSet::Locator::progress ()
  {
    _dboLocator->progress();
  }


  string  Cell::SlavedsSet::Locator::_getString () const
  {
    string s = "<" + _TName("Cell::SlavedsSet::Locator")
                   + getString(getElement())
                   + ">";
    return s;
  }


  Collection<Cell*>* Cell::SlavedsSet::getClone   () const
  { return new SlavedsSet(*this); }


  Locator<Cell*>* Cell::SlavedsSet::getLocator () const
  { return new Locator(_cell); }


  string  Cell::SlavedsSet::_getString () const
  {
    string s = "<" + _TName("Cell_SlavedsSet") + " "
                   + getString(_cell->getName())
                   + ">";
    return s;
  }


// ****************************************************************************************************
// Cell::InstanceMap implementation
// ****************************************************************************************************

Cell::InstanceMap::InstanceMap()
// *****************************
:    Inherit()
{
}

Name Cell::InstanceMap::_getKey(Instance* instance) const
// ******************************************************
{
    return instance->getName();
}

unsigned int  Cell::InstanceMap::_getHashValue(Name name) const
// *******************************************************
{
  return name._getSharedName()->getHash() / 8;
}

Instance* Cell::InstanceMap::_getNextElement(Instance* instance) const
// *******************************************************************
{
    return instance->_getNextOfCellInstanceMap();
}

void Cell::InstanceMap::_setNextElement(Instance* instance, Instance* nextInstance) const
// **************************************************************************************
{
    instance->_setNextOfCellInstanceMap(nextInstance);
}



// ****************************************************************************************************
// Cell::SlaveInstanceSet implementation
// ****************************************************************************************************

Cell::SlaveInstanceSet::SlaveInstanceSet()
// ***************************************
:    Inherit()
{
}

unsigned Cell::SlaveInstanceSet::_getHashValue(Instance* slaveInstance) const
// **************************************************************************
{
  return slaveInstance->getId() / 8;
}

Instance* Cell::SlaveInstanceSet::_getNextElement(Instance* slaveInstance) const
// *****************************************************************************
{
    return slaveInstance->_getNextOfCellSlaveInstanceSet();
}

void Cell::SlaveInstanceSet::_setNextElement(Instance* slaveInstance, Instance* nextSlaveInstance) const
// ****************************************************************************************************
{
    slaveInstance->_setNextOfCellSlaveInstanceSet(nextSlaveInstance);
}



// ****************************************************************************************************
// Cell::NetMap implementation
// ****************************************************************************************************

Cell::NetMap::NetMap()
// *******************
:    Inherit()
{
}

const Name& Cell::NetMap::_getKey(Net* net) const
// ***************************************
{
    return net->getName();
}

unsigned Cell::NetMap::_getHashValue(const Name& name) const
// *********************************************************
{
  unsigned long hash = 0;
  unsigned long sum4 = 0;
  const string& s = name._getSharedName()->_getSString();
  for ( size_t i=0 ; i<s.size() ; ++i ) {
    sum4 |= ((unsigned long)s[i]) << ((i%4) * 8);
    if (i%4 == 3) {
      hash += sum4;
      sum4  = 0;
    }
  }
  hash += sum4;

  return hash;
  
//return (unsigned int)name._getSharedName()->getId() / 8;
}

Net* Cell::NetMap::_getNextElement(Net* net) const
// ***********************************************
{
    return net->_getNextOfCellNetMap();
}

void Cell::NetMap::_setNextElement(Net* net, Net* nextNet) const
// *************************************************************
{
    net->_setNextOfCellNetMap(nextNet);
}


// ****************************************************************************************************
// Cell::PinMap implementation
// ****************************************************************************************************

Cell::PinMap::PinMap()
// *******************
:    Inherit()
{
}

Name Cell::PinMap::_getKey(Pin* pin) const
// ***************************************
{
    return pin->getName();
}

unsigned Cell::PinMap::_getHashValue(Name name) const
// **************************************************
{
  return (unsigned int)name._getSharedName()->getHash() / 8;
}

Pin* Cell::PinMap::_getNextElement(Pin* pin) const
// ***********************************************
{
    return pin->_getNextOfCellPinMap();
}

void Cell::PinMap::_setNextElement(Pin* pin, Pin* nextPin) const
// *************************************************************
{
    pin->_setNextOfCellPinMap(nextPin);
}


// ****************************************************************************************************
// Cell::SliceMap implementation
// ****************************************************************************************************

Cell::SliceMap::SliceMap()
// ***********************
:    Inherit()
{
}

const Layer* Cell::SliceMap::_getKey(Slice* slice) const
// *****************************************************
{
    return slice->getLayer();
}

unsigned Cell::SliceMap::_getHashValue(const Layer* layer) const
// *************************************************************
{
  return (unsigned int)layer->getMask() / 8;
}

Slice* Cell::SliceMap::_getNextElement(Slice* slice) const
// *******************************************************
{
    return slice->_getNextOfCellSliceMap();
}

void Cell::SliceMap::_setNextElement(Slice* slice, Slice* nextSlice) const
// ***********************************************************************
{
    slice->_setNextOfCellSliceMap(nextSlice);
};



// ****************************************************************************************************
// Cell::MarkerSet implementation
// ****************************************************************************************************

Cell::MarkerSet::MarkerSet()
// *************************
:    Inherit()
{
}

unsigned Cell::MarkerSet::_getHashValue(Marker* marker) const
// **********************************************************
{
  return (unsigned int)marker->getId() / 8;
}

Marker* Cell::MarkerSet::_getNextElement(Marker* marker) const
// ***********************************************************
{
    return marker->_getNextOfCellMarkerSet();
}

void Cell::MarkerSet::_setNextElement(Marker* marker, Marker* nextMarker) const
// ****************************************************************************
{
    marker->_setNextOfCellMarkerSet(nextMarker);
}



// ****************************************************************************************************
// JsonCell implementation
// ****************************************************************************************************


Initializer<JsonCell>  jsonCellInitialize ( 10 );


JsonCell::JsonCell(unsigned long flags)
// ************************************
  : JsonEntity(flags)
  , _cell                (NULL)
  , _materializationState(Go::autoMaterializationIsDisabled())
{
  remove( ".Cell" );
  add( "_library"     , typeid(string)    );
  add( "_name"        , typeid(string)    );
  add( "_abutmentBox" , typeid(Box)       );
  add( "+instanceMap" , typeid(JsonArray) );
  add( "+netMap"      , typeid(JsonArray) );

  Go::enableAutoMaterialization();
}

JsonCell::~JsonCell()
// ******************
{
  cdebug_log(19,0) << "JsonCell::~JsonCell() " << _cell << endl;

  Go::enableAutoMaterialization();
  if (_cell) _cell->materialize();

  if (_materializationState) Go::disableAutoMaterialization();
}

string JsonCell::getTypeName() const
// *********************************
{ return "Cell"; }

void  JsonCell::initialize()
// *************************
{ JsonTypes::registerType( new JsonCell (JsonWriter::RegisterMode) ); }

JsonCell* JsonCell::clone(unsigned long flags) const
// *************************************************
{ return new JsonCell ( flags ); }

void JsonCell::toData(JsonStack& stack)
// ************************************
{
  check( stack, "JsonCell::toData" );
  presetId( stack );

  Library* library = DataBase::getDB()->getLibrary( get<string>(stack,"_library")
                                                  , DataBase::CreateLib|DataBase::WarnCreateLib );
  _cell = Cell::create( library, get<string>(stack,"_name") );
  _cell->setAbutmentBox( stack.as<Box>("_abutmentBox") );

  update( stack, _cell );
}

} // End of Hurricane namespace.


// ****************************************************************************************************
// Copyright (c) BULL S.A. 2000-2018, All Rights Reserved
// ****************************************************************************************************
