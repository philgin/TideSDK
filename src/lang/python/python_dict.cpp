/**
 * Copyright (c) 2012 - 2014 TideSDK contributors
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include "python_dict.h"

namespace tide
{
    KPythonDict::KPythonDict(PyObject *obj) :
        TiObject("Python,KPythonDict"),
        object(obj)
    {
        PyLockGIL lock;
        Py_INCREF(this->object);
    }

    KPythonDict::~KPythonDict()
    {
        PyLockGIL lock;
        Py_DECREF(this->object);
        this->object = 0;
    }

    void KPythonDict::Set(const char* name, ValueRef value)
    {
        PyLockGIL lock;
        PyObject* pyval = PythonUtils::ToPyObject(value);
        int result = PyMapping_SetItemString(this->object, (char*)name, pyval);

        if (result == -1 && PyErr_Occurred() != 0)
        {
            THROW_PYTHON_EXCEPTION
        }
    }

    ValueRef KPythonDict::Get(const char *name)
    {
        PyLockGIL lock;

        if (PyMapping_HasKeyString(this->object, (char*) name) == 0)
            return Value::Undefined;

        PyObject* item = PyMapping_GetItemString(this->object, (char*) name);
        if (item == 0 && PyErr_Occurred())
        {
            THROW_PYTHON_EXCEPTION;
        }

        try
        {
            ValueRef value = PythonUtils::ToTiValue(item);
            Py_DECREF(item);
            return value;
        }
        catch (...)
        {
            Py_DECREF(item);
            throw;
        }
    }

    bool KPythonDict::Equals(TiObjectRef other)
    {
        AutoPtr<KPythonDict> pyOther = other.cast<KPythonDict>();

        // This is not a Python object
        if (pyOther.isNull())
            return false;

        return pyOther->ToPython() == this->ToPython();
    }

    SharedStringList KPythonDict::GetPropertyNames()
    {
        PyLockGIL lock;
        SharedStringList property_names = new StringList();

        // Avoid compiler warnings
        PyObject *items = PyObject_CallMethod(this->object, (char*) "items", 0);
        if (items == 0)
            return property_names;

        PyObject *iterator = PyObject_GetIter(items);
        if (iterator == 0)
            return property_names;

        PyObject *item;
        while ((item = PyIter_Next(iterator))) {
            PyObject* k = PyTuple_GetItem(item, 0);
            std::string sk = PythonUtils::ToString(k);
            property_names->push_back(new std::string(sk));
            Py_DECREF(item);
        }

        Py_DECREF(iterator);
        Py_DECREF(items);
        return property_names;
    }

    PyObject* KPythonDict::ToPython()
    {
        PyLockGIL lock;
        Py_INCREF(object);
        return this->object;
    }
}
