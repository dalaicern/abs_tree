#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "types.h"

static Ty_ty Ty_New(int kind)
{
    Ty_ty t = checked_malloc(sizeof(*t));
    t->kind = kind;
    return t;
}

Ty_ty Ty_Nil(void)
{
    return Ty_New(Ty_nil);
}

Ty_ty Ty_Int(void)
{
    return Ty_New(Ty_int);
}

Ty_ty Ty_String(void)
{
    return Ty_New(Ty_string);
}

Ty_ty Ty_Void(void)
{
    return Ty_New(Ty_void);
}

Ty_ty Ty_Record(Ty_fieldList fields)
{
    Ty_ty t = Ty_New(Ty_record);
    t->u.record = fields;
    return t;
}

Ty_ty Ty_Array(Ty_ty ty)
{
    Ty_ty t = Ty_New(Ty_array);
    t->u.array = ty;
    return t;
}

Ty_ty Ty_Name(S_symbol sym, Ty_ty ty)
{
    Ty_ty t = Ty_New(Ty_name);
    t->u.name.sym = sym;
    t->u.name.ty = ty;
    return t;
}

Ty_tyList Ty_TyList(Ty_ty head, Ty_tyList tail)
{
    Ty_tyList t = checked_malloc(sizeof(*t));
    t->head = head;
    t->tail = tail;
    return t;
}

Ty_field Ty_Field(S_symbol name, Ty_ty ty)
{
    Ty_field f = checked_malloc(sizeof(*f));
    f->name = name;
    f->ty = ty;
    return f;
}

Ty_fieldList Ty_FieldList(Ty_field head, Ty_fieldList tail)
{
    Ty_fieldList t = checked_malloc(sizeof(*t));
    t->head = head;
    t->tail = tail;
    return t;
}