/**
 * Copyright (c) 2012 - 2014 TideSDK contributors
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include "php_list.h"

namespace tide
{
    KPHPList::KPHPList(zval* list) :
        TiList("PHP.KPHPList"),
        list(list)
    {
        if (Z_TYPE_P(list) != IS_ARRAY)
            throw ValueException::FromString("Invalid zval passed. Should be an array type.");
    }

    KPHPList::~KPHPList()
    {
    }

    ValueRef KPHPList::Get(const char* name)
    {
        if (TiList::IsInt(name))
        {
            return this->At(TiList::ToIndex(name));
        }

        unsigned long hashval = zend_get_hash_value((char *) name, strlen(name));
        zval **copyval;

        if (zend_hash_quick_find(Z_ARRVAL_P(this->list),
                    (char *) name,
                    strlen(name),
                    hashval,
                    (void**)&copyval) == FAILURE)
        {
            return Value::Undefined;
        }

        TSRMLS_FETCH();
        ValueRef v = PHPUtils::ToTiValue((zval *) copyval TSRMLS_CC);
        return v;
    }

    void KPHPList::Set(const char* name, ValueRef value)
    {
        // Check for integer value as name
        if (TiList::IsInt(name))
        {
            this->SetAt(TiList::ToIndex(name), value);
        }
        else
        {
            AddTideValueToPHPArray(value, this->list, name);
        }
    }

    bool KPHPList::Equals(TiObjectRef other)
    {
        AutoPtr<KPHPList> phpOther = other.cast<KPHPList>();

        // This is not a PHP object
        if (phpOther.isNull())
            return false;

        // Do an identity (===) comparison on the two hashes
        TSRMLS_FETCH();
        return zend_hash_compare(
            Z_ARRVAL_P(phpOther->ToPHP()), Z_ARRVAL_P(this->ToPHP()), 
            (compare_func_t) PHPUtils::HashZvalCompareCallback, 1 TSRMLS_CC) == 0;
    }

    SharedStringList KPHPList::GetPropertyNames()
    {
        return PHPUtils::GetHashKeys(Z_ARRVAL_P(this->list));
    }

    unsigned int KPHPList::Size()
    {
        return (unsigned int) zend_hash_num_elements(Z_ARRVAL_P(this->list));
    }

    void KPHPList::Append(ValueRef value)
    {
        AddTideValueToPHPArray(value, this->list);
    }

    void KPHPList::SetAt(unsigned int index, ValueRef value)
    {
        AddTideValueToPHPArray(value, this->list, index);
    }

    bool KPHPList::Remove(unsigned int index)
    {
        if (index < this->Size())
        {
            if (zend_hash_index_del(Z_ARRVAL_P(this->list), (unsigned long) index) == SUCCESS)
                return true;
        }

        return false;
    }

    ValueRef KPHPList::At(unsigned int index)
    {
        zval **copyval;

        if (zend_hash_index_find(Z_ARRVAL_P(this->list),
                index, (void**)&copyval) == FAILURE)
        {
            return Value::Undefined;
        }

        TSRMLS_FETCH();
        ValueRef v = PHPUtils::ToTiValue((zval *) copyval TSRMLS_CC);
        return v;
    }

    zval* KPHPList::ToPHP()
    {
        return this->list;
    }

    void KPHPList::AddTideValueToPHPArray(ValueRef value, zval *phpArray, const char* key)
    {
        if (value->IsNull() || value->IsUndefined())
        {
            add_assoc_null(phpArray, (char *) key);
        }
        else if (value->IsBool())
        {
            if (value->ToBool())
                add_assoc_bool(phpArray, (char *) key, 1);
            else
                add_assoc_bool(phpArray, (char *) key, 0);
        }
        else if (value->IsNumber())
        {
            /* No way to check whether the number is an
               integer or a double here. All Tide numbers
               are doubles, so return a double. This could
               cause some PHP to function incorrectly if it's
               doing strict type checking. */
            add_assoc_double(phpArray, (char *) key, value->ToNumber());
        }
        else if (value->IsString())
        {
            add_assoc_stringl(phpArray, (char *) key, (char *) value->ToString(), strlen(value->ToString()), 1);
        }
        else if (value->IsObject())
        {
            /*TODO: Implement*/
        }
        else if (value->IsMethod())
        {
            /*TODO: Implement*/
        }
        else if (value->IsList())
        {
            zval *phpValue;
            AutoPtr<KPHPList> pl = value->ToList().cast<KPHPList>();
            if (!pl.isNull())
                phpValue = pl->ToPHP();
            else
                phpValue = PHPUtils::ToPHPValue(value);

            add_assoc_zval(phpArray, (char *) key, phpValue);
        }
    }

    void KPHPList::AddTideValueToPHPArray(ValueRef value, zval *phpArray, unsigned int index)
    {
        if (value->IsNull() || value->IsUndefined())
        {
            add_index_null(phpArray, (unsigned long) index);
        }
        else if (value->IsBool())
        {
            if (value->ToBool())
                add_index_bool(phpArray, (unsigned long) index, 1);
            else
                add_index_bool(phpArray, (unsigned long) index, 0);
        }
        else if (value->IsNumber())
        {
            /* No way to check whether the number is an
               integer or a double here. All Tide numbers
               are doubles, so return a double. This could
               cause some PHP to function incorrectly if it's
               doing strict type checking. */
            add_index_double(phpArray, (unsigned long) index, value->ToNumber());
        }
        else if (value->IsString())
        {
            add_index_stringl(phpArray, (unsigned long) index, (char *) value->ToString(), strlen(value->ToString()), 1);
        }
        else if (value->IsObject())
        {
            /*TODO: Implement*/
        }
        else if (value->IsMethod())
        {
            /*TODO: Implement*/
        }
        else if (value->IsList())
        {
            zval *phpValue;
            AutoPtr<KPHPList> pl = value->ToList().cast<KPHPList>();
            if (!pl.isNull())
                phpValue = pl->ToPHP();
            else
                phpValue = PHPUtils::ToPHPValue(value);

            add_index_zval(phpArray, (unsigned long) index, phpValue);
        }
    }

    void KPHPList::AddTideValueToPHPArray(ValueRef value, zval *phpArray)
    {
        AddTideValueToPHPArray(value, phpArray, (unsigned int) zend_hash_num_elements(Z_ARRVAL_P(phpArray)));
    }
}
