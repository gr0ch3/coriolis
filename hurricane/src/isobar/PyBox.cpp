// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+ 
// |                   C O R I O L I S                               |
// |    I s o b a r  -  Hurricane / Python Interface                 |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./PyBox.cpp"                              |
// +-----------------------------------------------------------------+


#include "hurricane/isobar/PyPoint.h"
#include "hurricane/isobar/PyBox.h"


namespace  Isobar {

using namespace Hurricane;

extern "C" {


#define  METHOD_HEAD(function)  GENERIC_METHOD_HEAD(Box,box,function)


// +=================================================================+
// |                 "PyBox" Python Module Code Part                 |
// +=================================================================+

#if defined(__PYTHON_MODULE__)


  // Standart Accessors (Attributes).
  DirectGetLongAttribute(PyBox_getXMin      ,getXMin      ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getYMin      ,getYMin      ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getXMax      ,getXMax      ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getYMax      ,getYMax      ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getXCenter   ,getXCenter   ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getYCenter   ,getYCenter   ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getWidth     ,getWidth     ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getHalfWidth ,getHalfWidth ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getHeight    ,getHeight    ,PyBox,Box)
  DirectGetLongAttribute(PyBox_getHalfHeight,getHalfHeight,PyBox,Box)


  // Standart Predicates (Attributes).
  DirectGetBoolAttribute(PyBox_isEmpty   ,isEmpty   ,PyBox,Box)
  DirectGetBoolAttribute(PyBox_isFlat    ,isFlat    ,PyBox,Box)
  DirectGetBoolAttribute(PyBox_isPonctual,isPonctual,PyBox,Box)


  // Standart Destroy (Attribute).
  DirectDestroyAttribute(PyBox_destroy, PyBox)
  

  static PyObject* PyBox_NEW (PyObject *module, PyObject *args) {
    cdebug_log(20,0) << "PyBox_NEW()" << endl;

    Box* box = NULL;
    PyBox* pyBox = NULL;

    HTRY
    PyObject* arg0;
    PyObject* arg1;
    PyObject* arg2;
    PyObject* arg3;
    __cs.init ("Box.Box");

    if (! PyArg_ParseTuple(args,"|O&O&O&O&:Box.Box",
                Converter, &arg0,
                Converter, &arg1,
                Converter, &arg2,
                Converter, &arg3)) {
        return NULL;
    }

    if  (__cs.getObjectIds() == NO_ARG) { box = new Box (); }
    else if ( __cs.getObjectIds() == POINT_ARG   ) { box = new Box ( *PYPOINT_O(arg0) ); }
    else if ( __cs.getObjectIds() == BOX_ARG     ) { box = new Box ( *PYBOX_O(arg0) ); }
    else if ( __cs.getObjectIds() == POINTS2_ARG ) { box = new Box ( *PYPOINT_O(arg0) , *PYPOINT_O(arg1) ); }
    else if ( __cs.getObjectIds() == INTS2_ARG   ) { box = new Box ( PyAny_AsLong(arg0) , PyAny_AsLong(arg1) ); }
    else if ( __cs.getObjectIds() == INTS4_ARG   ) {
        box = new Box ( PyAny_AsLong(arg0), PyAny_AsLong(arg1), PyAny_AsLong(arg2) , PyAny_AsLong(arg3) );
    } else {
        PyErr_SetString(ConstructorError, "invalid number of parameters for Box constructor." );
        return NULL;
    }

    pyBox = PyObject_NEW(PyBox, &PyTypeBox);
    if (pyBox == NULL) return NULL;

    pyBox->_object = box;
    HCATCH

    return ( (PyObject*)pyBox );
  }


  static int  PyBox_Init ( PyBox* self, PyObject* args, PyObject* kwargs )
  {
    cdebug_log(20,0) << "PyBox_Init(): " << (void*)self << endl;
    return 0;
  }


  static PyObject* PyBox_getCenter ( PyBox *self ) {
    cdebug_log(20,0) << "PyBox_getCenter()" << endl;

    METHOD_HEAD ( "Box.Center()" )

    PyPoint* pyPoint = PyObject_NEW ( PyPoint, &PyTypePoint );
    if (pyPoint == NULL) { return NULL; }
    
    HTRY
    pyPoint->_object = new Point ( box->getCenter() );
    HCATCH    

    return (PyObject*)pyPoint;
  }


  static PyObject* PyBox_getUnion ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_getUnion()" << endl;

    METHOD_HEAD ( "Box.getUnion()" )

    PyBox* otherPyBox;
    if (PyArg_ParseTuple(args,"O!:Box.getUnion", &PyTypeBox, &otherPyBox)) {
        PyBox* unionPyBox = PyObject_NEW ( PyBox, &PyTypeBox );
        if (unionPyBox == NULL) {
            return NULL;
        }
        HTRY
        unionPyBox->_object = new Box ( box->getUnion(*PYBOX_O(otherPyBox)));
        HCATCH
        return (PyObject*)unionPyBox;
    } else {
        PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.getUnion.");
    }
    return NULL;

  }


  static PyObject* PyBox_getIntersection ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_getIntersection()" << endl;

    METHOD_HEAD ( "Box.getIntersection()" )

    PyBox* otherPyBox;
    if (PyArg_ParseTuple(args,"O!:Box.getIntersection", &PyTypeBox, &otherPyBox)) {
        PyBox* intersectionPyBox = PyObject_NEW ( PyBox, &PyTypeBox );
        if (intersectionPyBox == NULL) {
            return NULL;
        }
        HTRY
        intersectionPyBox->_object = new Box(box->getIntersection(*PYBOX_O(otherPyBox)));
        HCATCH
        return (PyObject*)intersectionPyBox;
    } else {
        PyErr_SetString(ConstructorError, "invalid number of parameters for Box.getIntersection.");
    }
    return NULL;
  }


  static PyObject* PyBox_contains ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_contains ()" << endl;

    METHOD_HEAD ( "Box.contains()" )

    PyObject* arg0;
    PyObject* arg1;
    bool result = false;

    HTRY

    __cs.init ("Box.contains");
    if ( ! PyArg_ParseTuple(args,"|O&O&:Box.contains",Converter,&arg0,Converter,&arg1) )
        return NULL;

    if      ( __cs.getObjectIds() == BOX_ARG   ) { result = box->contains ( *PYBOX_O(arg0) ); }
    else if ( __cs.getObjectIds() == POINT_ARG ) { result = box->contains ( *PYPOINT_O(arg0) ); }
    else if ( __cs.getObjectIds() == INTS2_ARG ) { result = box->contains ( PyAny_AsLong(arg0)
                                                                          , PyAny_AsLong(arg1) ); }
    else {
      PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.contains constructor." );
      return NULL;
    }

    HCATCH
    if (result) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
  }


  static PyObject* PyBox_intersect ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_intersect ()" << endl;
    
    bool result = false;
    HTRY
    PyBox* pyBox = NULL;
    METHOD_HEAD ( "Box.intersect()" )

    if (PyArg_ParseTuple(args,"O!:Box.intersects", &PyTypeBox, &pyBox)) {
        result = box->intersect(*PYBOX_O(pyBox));
    } else {
      PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.intersect." );
      return NULL;
    }

    HCATCH
    if (result) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }

  }


  static PyObject* PyBox_isConstrainedBy ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_isConstrainedBy ()" << endl;
    
    bool result = false;
    HTRY
    PyBox* pyBox = NULL;
    METHOD_HEAD("Box.isConstrainedBy()")

    if (PyArg_ParseTuple(args,"O!:Box.isConstrainedBy", &PyTypeBox, &pyBox)) {
        result = box->isConstrainedBy(*PYBOX_O(pyBox));
    } else {
      PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.isConstrainedBy.");
      return NULL;
    }

    HCATCH
    if (result) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
  }


  static PyObject* PyBox_makeEmpty ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_makeEmpty ()" << endl;

    HTRY
    METHOD_HEAD ( "Box.makeEmpty()" )

    box->makeEmpty ();
    HCATCH
    
    Py_INCREF ( self ); //FIXME ??
    return (PyObject*)self;
  }


  static PyObject* PyBox_inflate ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_inflate ()" << endl;

    METHOD_HEAD ( "Box.inflate()" )

    PyObject* arg0;
    PyObject* arg1;
    PyObject* arg2;
    PyObject* arg3;

    HTRY

    __cs.init ("Box.inflate");
    if ( ! PyArg_ParseTuple(args,"|O&O&O&O&:Box.inflate",Converter,&arg0,Converter,&arg1,Converter,&arg2,Converter,&arg3) )
      return ( NULL );

    if      ( __cs.getObjectIds() == INT_ARG   ) { box->inflate ( PyAny_AsLong(arg0) ); }
    else if ( __cs.getObjectIds() == INTS2_ARG ) { box->inflate ( PyAny_AsLong(arg0)
                                                                , PyAny_AsLong(arg1) ); }
    else if ( __cs.getObjectIds() == INTS4_ARG ) { box->inflate ( PyAny_AsLong(arg0)
                                                                , PyAny_AsLong(arg1)
                                                                , PyAny_AsLong(arg2)
                                                                , PyAny_AsLong(arg3) ); }
    else {
      PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.inflate()" );
      return ( NULL );
    }

    HCATCH

    Py_INCREF ( self );
    return ( (PyObject*)self );
  }


  static PyObject* PyBox_merge ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "Box_merge()" << endl;

    METHOD_HEAD ( "Box.merge()" )

    PyObject* arg0;
    PyObject* arg1;
    PyObject* arg2;
    PyObject* arg3;

    HTRY

    __cs.init ("Box.merge");
    if ( ! PyArg_ParseTuple(args,"|O&O&O&O&:Box.merge",Converter,&arg0,Converter,&arg1,Converter,&arg2,Converter,&arg3) )
      return ( NULL );

    if      ( __cs.getObjectIds() == POINT_ARG ) { box->merge ( *PYPOINT_O(arg0) ); }
    else if ( __cs.getObjectIds() == BOX_ARG   ) { box->merge ( *PYBOX_O(arg0) ); }
    else if ( __cs.getObjectIds() == INTS2_ARG ) { box->merge ( PyAny_AsLong(arg0)
                                                              , PyAny_AsLong(arg1) ); }
    else if ( __cs.getObjectIds() == INTS4_ARG ) { box->merge ( PyAny_AsLong(arg0)
                                                              , PyAny_AsLong(arg1)
                                                              , PyAny_AsLong(arg2)
                                                              , PyAny_AsLong(arg3) ); }
    else {
      PyErr_SetString ( ConstructorError, "invalid number of parameters for Box.merge()" );
      return ( NULL );
    }

    HCATCH

    Py_INCREF ( self );
    return ( (PyObject*)self );
  }


  static PyObject* PyBox_translate ( PyBox *self, PyObject* args ) {
    cdebug_log(20,0) << "PyBox_translate ()" << endl;
    
    HTRY
    METHOD_HEAD ( "Box.translate()" )
    PyObject* arg0 = NULL;
    PyObject* arg1 = NULL;
    __cs.init ("Box.translate");
    if (PyArg_ParseTuple(args,"O&|O&:Box.translate", Converter, &arg0, Converter, &arg1)) {
      if      (__cs.getObjectIds() == INTS2_ARG) box->translate( PyAny_AsLong(arg0), PyAny_AsLong(arg1) );
      else if (__cs.getObjectIds() == POINT_ARG) box->translate( *PYPOINT_O(arg0) );
      else {
        PyErr_SetString ( ConstructorError, "Box.translate(): Invalid type for parameter(s)." );
        return NULL;
      }
    } else {
      PyErr_SetString ( ConstructorError, "Box.translate(): Invalid number of parameters." );
      return NULL;
    }
    HCATCH

    Py_INCREF ( self );
    return ( (PyObject*)self );
  }


  // ---------------------------------------------------------------
  // PyBox Attribute Method table.

  PyMethodDef PyBox_Methods[] =
    { { "getXMin"        , (PyCFunction)PyBox_getXMin        , METH_NOARGS , "Return the XMin value." }
    , { "getYMin"        , (PyCFunction)PyBox_getYMin        , METH_NOARGS , "Return the YMin value." }
    , { "getXMax"        , (PyCFunction)PyBox_getXMax        , METH_NOARGS , "Return the XMax value." }
    , { "getYMax"        , (PyCFunction)PyBox_getYMax        , METH_NOARGS , "Return the YMax value." }
    , { "getXCenter"     , (PyCFunction)PyBox_getXCenter     , METH_NOARGS , "Return the X box center value." }
    , { "getYCenter"     , (PyCFunction)PyBox_getYCenter     , METH_NOARGS , "Return the Y box center value." }
    , { "getCenter"      , (PyCFunction)PyBox_getCenter      , METH_NOARGS , "Return the box center Point." }
    , { "getWidth"       , (PyCFunction)PyBox_getWidth       , METH_NOARGS , "Return the box width." }
    , { "getHalfWidth"   , (PyCFunction)PyBox_getHalfWidth   , METH_NOARGS , "Return the box half width." }
    , { "getHeight"      , (PyCFunction)PyBox_getHeight      , METH_NOARGS , "Return the box height." }
    , { "getHalfHeight"  , (PyCFunction)PyBox_getHalfHeight  , METH_NOARGS , "Return the box half height." }
    , { "getUnion"       , (PyCFunction)PyBox_getUnion       , METH_VARARGS, "Return the smallest enclosing box." }
    , { "getIntersection", (PyCFunction)PyBox_getIntersection, METH_VARARGS, "Return the overlapping area." }
    , { "isEmpty"        , (PyCFunction)PyBox_isEmpty        , METH_NOARGS , "Return true if the box is empty." }
    , { "isFlat"         , (PyCFunction)PyBox_isFlat         , METH_NOARGS , "Return true if the box is flat." }
    , { "isPonctual"     , (PyCFunction)PyBox_isPonctual     , METH_NOARGS , "Return true if the box reduced to a point." }
    , { "contains"       , (PyCFunction)PyBox_contains       , METH_VARARGS, "Return true if the box contains the argument." }
    , { "intersect"      , (PyCFunction)PyBox_intersect      , METH_VARARGS, "Return true if two boxes overlap." }
    , { "isConstrainedBy", (PyCFunction)PyBox_isConstrainedBy, METH_VARARGS, "Return true if the argment box is included and share at least a side." }
    , { "makeEmpty"      , (PyCFunction)PyBox_makeEmpty      , METH_NOARGS , "Transform the box in an empty one." }
    , { "inflate"        , (PyCFunction)PyBox_inflate        , METH_VARARGS, "Expand the box to contains the arguments." }
    , { "merge"          , (PyCFunction)PyBox_merge          , METH_VARARGS, "Expand or contract the box to contains the arguments." }
    , { "translate"      , (PyCFunction)PyBox_translate      , METH_VARARGS, "translate the box od dx ans dy." }
    , { "destroy"        , (PyCFunction)PyBox_destroy        , METH_NOARGS
                         , "Destroy associated hurricane object, the python object remains." }
    , {NULL, NULL, 0, NULL}           /* sentinel */
    };


  // +-------------------------------------------------------------+
  // |                  "PyBox" Object Methods                     |
  // +-------------------------------------------------------------+


  DirectDeleteMethod(PyBox_DeAlloc,PyBox)
  PyTypeObjectLinkPyTypeAsValue(Box)

#else  // End of Python Module Code Part.


// +=================================================================+
// |                "PyBox" Shared Library Code Part                 |
// +=================================================================+

  
  PyTypeObjectDefinitions(Box)

# endif  // End of Shared Library Code Part.

}  // End of extern "C".

}  // End of Isobar namespace.
 
