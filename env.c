#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"

E_enventry E_VarEntry(Ty_ty ty) {
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_varEntry;
    entry->u.var.ty = ty;
    return entry;
}

E_enventry E_FunEntry(Ty_tyList formals, Ty_ty result) {
    E_enventry entry = checked_malloc(sizeof(*entry));
    entry->kind = E_funEntry;
    entry->u.fun.formals = formals;
    entry->u.fun.result = result;
    return entry;
}

S_table E_base_tenv(void) {
    S_table tenv = S_empty();
    
    // Add predefined types (int and string)
    S_enter(tenv, S_Symbol("int"), Ty_Int());
    S_enter(tenv, S_Symbol("string"), Ty_String());
    
    return tenv;
}

S_table E_base_venv(void) {
    S_table venv = S_empty();
    
    // Add Tiger's built-in functions to the value environment
    
    // print(s: string)
    S_enter(venv, S_Symbol("print"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Void()));
    
    // flush()
    S_enter(venv, S_Symbol("flush"),
            E_FunEntry(NULL, Ty_Void()));
    
    // getchar(): string
    S_enter(venv, S_Symbol("getchar"),
            E_FunEntry(NULL, Ty_String()));
    
    // ord(s: string): int
    S_enter(venv, S_Symbol("ord"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
    
    // chr(i: int): string
    S_enter(venv, S_Symbol("chr"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_String()));
    
    // size(s: string): int
    S_enter(venv, S_Symbol("size"),
            E_FunEntry(Ty_TyList(Ty_String(), NULL), Ty_Int()));
    
    // substring(s: string, first: int, n: int): string
    S_enter(venv, S_Symbol("substring"),
            E_FunEntry(Ty_TyList(Ty_String(), 
                      Ty_TyList(Ty_Int(), 
                      Ty_TyList(Ty_Int(), NULL))), Ty_String()));
    
    // concat(s1: string, s2: string): string
    S_enter(venv, S_Symbol("concat"),
            E_FunEntry(Ty_TyList(Ty_String(), 
                      Ty_TyList(Ty_String(), NULL)), Ty_String()));
    
    // not(i: int): int
    S_enter(venv, S_Symbol("not"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Int()));
    
    // exit(i: int)
    S_enter(venv, S_Symbol("exit"),
            E_FunEntry(Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    
    return venv;
}