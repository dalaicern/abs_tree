#include "semant.h"
#include "env.h"
#include <stddef.h> 
#include <assert.h>
#include "errormsg.h"

void SEM_transProg(A_exp exp) {
    // Create and initialize the base type and value environments
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    
    // Perform type checking on the entire program
    transExp(venv, tenv, exp);
}

Ty_ty actual_ty(Ty_ty ty) {
    if (!ty) return NULL;
    
    if (ty->kind == Ty_name) {
        Ty_ty t = ty->u.name.ty;
        if (t == NULL) {
            return ty;
        }
        return actual_ty(t);
    }
    else {
        return ty;
    }
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList params) {
    if (!params) {
        return NULL;
    }
    
    Ty_ty ty = S_look(tenv, params->head->typ);
    if (!ty) {
        EM_error(params->head->pos, "undefined type %s", 
                 S_name(params->head->typ));
        ty = Ty_Int();
    }
    
    return Ty_TyList(ty, makeFormalTyList(tenv, params->tail));
}

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch(v->kind) {
        case A_simpleVar: {
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry)
                return expTy(NULL, actual_ty(x->u.var.ty));
            else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_fieldVar:
            break;
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    switch(a->kind) {
        case A_varExp:
            return transVar(venv, tenv, a->u.var);
            
        case A_nilExp:
            return expTy(NULL, Ty_Nil());
            
        case A_intExp:
            return expTy(NULL, Ty_Int());
            
        case A_stringExp:
            return expTy(NULL, Ty_String());
            
        case A_callExp: {
            E_enventry func = S_look(venv, a->u.call.func);
            if (!func || func->kind != E_funEntry) {
                EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Int()); // Default to int for error recovery
            }
            
            // Check that argument count matches
            A_expList args = a->u.call.args;
            Ty_tyList formals = func->u.fun.formals;
            
            while (args && formals) {
                struct expty arg = transExp(venv, tenv, args->head);
                if (formals->head->kind != arg.ty->kind) {
                    EM_error(args->head->pos, "argument type mismatch");
                }
                args = args->tail;
                formals = formals->tail;
            }
            
            if (args) {
                EM_error(a->pos, "too many arguments");
            } else if (formals) {
                EM_error(a->pos, "too few arguments");
            }
            
            return expTy(NULL, func->u.fun.result);
        }
        
        case A_opExp: {
            A_oper oper = a->u.op.oper;
            struct expty left = transExp(venv, tenv, a->u.op.left);
            struct expty right = transExp(venv, tenv, a->u.op.right);
            if (oper == A_plusOp || oper == A_minusOp || 
                oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int)
                    EM_error(a->u.op.left->pos, "integer required");
                if (right.ty->kind != Ty_int)
                    EM_error(a->u.op.right->pos, "integer required");
                return expTy(NULL, Ty_Int());
            }
            
            else if (oper == A_eqOp || oper == A_neqOp) {
                if ((left.ty->kind == Ty_int && right.ty->kind == Ty_int) ||
                    (left.ty->kind == Ty_string && right.ty->kind == Ty_string)) {
                    return expTy(NULL, Ty_Int());
                } else if (left.ty->kind == Ty_record && 
                        (right.ty->kind == Ty_record || right.ty->kind == Ty_nil)) {
                    return expTy(NULL, Ty_Int());
                }
                else if (right.ty->kind == Ty_record && 
                        (left.ty->kind == Ty_record || left.ty->kind == Ty_nil)) {
                    return expTy(NULL, Ty_Int());
                }
                else if (left.ty->kind == Ty_array && right.ty->kind == Ty_array && 
                        left.ty->u.array == right.ty->u.array) {
                    return expTy(NULL, Ty_Int());
                }
                else {
                    EM_error(a->pos, "incomparable types for equality operator");
                    return expTy(NULL, Ty_Int());
                }
            }
            
            else if (oper == A_ltOp || oper == A_leOp || 
                    oper == A_gtOp || oper == A_geOp) {
                if (left.ty->kind == Ty_int && right.ty->kind == Ty_int) {
                    return expTy(NULL, Ty_Int());
                }
                else if (left.ty->kind == Ty_string && right.ty->kind == Ty_string) {
                    return expTy(NULL, Ty_Int());
                }
                else {
                    EM_error(a->pos, "integer or string required for comparison operator");
                    return expTy(NULL, Ty_Int());
                }
            }
            
            else {
                EM_error(a->pos, "unknown operator");
                return expTy(NULL, Ty_Int());
            }
            break;
        }
        case A_recordExp: {
            // Look up the record type
            Ty_ty record_ty = S_look(tenv, a->u.record.typ);
            if (!record_ty) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Int());
            }
            
            // Resolve name types 
            record_ty = actual_ty(record_ty);
            
            if (record_ty->kind != Ty_record) {
                EM_error(a->pos, "%s is not a record type", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            }
            
            // Check field types
            A_efieldList fields = a->u.record.fields;
            Ty_fieldList ty_fields = record_ty->u.record;
            
            while (fields && ty_fields) {
                // Find matching field by name
                Ty_fieldList tf = ty_fields;
                bool found = false;
                
                while (tf && !found) {
                    if (fields->head->name == tf->head->name) {
                        // Check field type
                        struct expty field_exp = transExp(venv, tenv, fields->head->exp);
                        if (field_exp.ty->kind != tf->head->ty->kind) {
                            EM_error(a->pos, "field %s has wrong type", 
                                    S_name(fields->head->name));
                        }
                        found = true;
                    }
                    tf = tf->tail;
                }
                
                if (!found) {
                    EM_error(a->pos, "field %s not found in record type", 
                            S_name(fields->head->name));
                }
                
                fields = fields->tail;
            }
            
            return expTy(NULL, record_ty);
        }
        
        case A_seqExp: {
            A_expList seq = a->u.seq;
            struct expty result = expTy(NULL, Ty_Void());
            
            // Process each expression in sequence and return type of last one
            while (seq) {
                result = transExp(venv, tenv, seq->head);
                seq = seq->tail;
            }
            
            return result;
        }
        
        case A_assignExp: {
            struct expty var = transVar(venv, tenv, a->u.assign.var);
            struct expty exp = transExp(venv, tenv, a->u.assign.exp);
            
            // Check type compatibility
            if (var.ty->kind != exp.ty->kind && 
                !(var.ty->kind == Ty_record && exp.ty->kind == Ty_nil)) {
                EM_error(a->pos, "type mismatch in assignment");
            }
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_ifExp: {
            struct expty test = transExp(venv, tenv, a->u.iff.test);
            struct expty then = transExp(venv, tenv, a->u.iff.then);
            
            // Test condition must be integer (Boolean)
            if (test.ty->kind != Ty_int) {
                EM_error(a->u.iff.test->pos, "integer (boolean) required for condition");
            }
            
            // Handle optional else clause
            if (a->u.iff.elsee) {
                struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
                
                // Then and else must have same type
                if (then.ty->kind != elsee.ty->kind) {
                    EM_error(a->pos, "then and else expressions must have same type");
                }
                
                return expTy(NULL, then.ty);
            } else {
                // Without else, the result is void
                return expTy(NULL, Ty_Void());
            }
        }
        
        case A_whileExp: {
            struct expty test = transExp(venv, tenv, a->u.whilee.test);
            struct expty body = transExp(venv, tenv, a->u.whilee.body);
            
            // Test condition must be integer (Boolean)
            if (test.ty->kind != Ty_int) {
                EM_error(a->u.whilee.test->pos, "integer (boolean) required for condition");
            }
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_forExp: {
            struct expty lo = transExp(venv, tenv, a->u.forr.lo);
            struct expty hi = transExp(venv, tenv, a->u.forr.hi);
            
            // Range bounds must be integers
            if (lo.ty->kind != Ty_int) {
                EM_error(a->u.forr.lo->pos, "integer required for lower bound");
            }
            if (hi.ty->kind != Ty_int) {
                EM_error(a->u.forr.hi->pos, "integer required for upper bound");
            }
            
            // Add loop variable to scope
            S_beginScope(venv);
            S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Int()));
            
            // Type check the body
            transExp(venv, tenv, a->u.forr.body);
            
            S_endScope(venv);
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_breakExp:
            return expTy(NULL, Ty_Void());
            
        case A_letExp: {
            struct expty exp;
            A_decList d;
            S_beginScope(venv);
            S_beginScope(tenv);
            for (d = a->u.let.decs; d; d = d->tail)
                transDec(venv, tenv, d->head);
            exp = transExp(venv, tenv, a->u.let.body);
            S_endScope(tenv);
            S_endScope(venv);
            return exp;
        }
        
        case A_arrayExp: {
            Ty_ty array_ty = S_look(tenv, a->u.array.typ);
            if (!array_ty) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.array.typ));
                return expTy(NULL, Ty_Array(Ty_Int()));
            }
            
            array_ty = actual_ty(array_ty);
            
            if (array_ty->kind != Ty_array) {
                EM_error(a->pos, "%s is not an array type", S_name(a->u.array.typ));
                return expTy(NULL, Ty_Array(Ty_Int()));
            }
            
            // Size must be integer
            struct expty size = transExp(venv, tenv, a->u.array.size);
            if (size.ty->kind != Ty_int) {
                EM_error(a->u.array.size->pos, "integer required for array size");
            }
            
            // Check initializer type
            struct expty init = transExp(venv, tenv, a->u.array.init);
            if (init.ty->kind != array_ty->u.array->kind) {
                EM_error(a->u.array.init->pos, "type mismatch in array initializer");
            }
            
            return expTy(NULL, array_ty);
        }
    }

    assert(0); // Should never reach here
    return expTy(NULL, Ty_Void());
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    switch(d->kind) {
        case A_varDec: {
            struct expty e = transExp(venv, tenv, d->u.var.init);
            S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
            break;
        }
        case A_typeDec: {
            S_enter(tenv, d->u.type->head->name,
                    transTy(tenv, d->u.type->head->ty));
            break;
        }
        case A_functionDec: {
            A_fundec f = d->u.function->head;
            Ty_ty resultTy = S_look(tenv, f->result);
            Ty_tyList formalTys = makeFormalTyList(tenv, f->params);
            S_enter(venv, f->name, E_FunEntry(formalTys, resultTy));
            S_beginScope(venv);
            {
                A_fieldList l;
                Ty_tyList t;
                for (l = f->params, t = formalTys; l; l = l->tail, t = t->tail)
                    S_enter(venv, l->head->name, E_VarEntry(t->head));
            }
            transExp(venv, tenv, f->body);
            S_endScope(venv);
            break;
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    switch(a->kind) {
        case A_nameTy: {
            Ty_ty ty = S_look(tenv, a->u.name);
            if (ty)
                return ty;
            else {
                EM_error(a->pos, "undefined type %s", S_name(a->u.name));
                return Ty_Int(); /* default to int for undefined types */
            }
        }
        
        case A_recordTy: {
            A_fieldList fields = a->u.record;
            Ty_fieldList ty_fields = NULL, tail = NULL;
            
            for (; fields; fields = fields->tail) {
                Ty_ty field_type = S_look(tenv, fields->head->typ);
                if (!field_type) {
                    EM_error(fields->head->pos, "undefined type %s", 
                            S_name(fields->head->typ));
                    field_type = Ty_Int();
                }
                
                Ty_field ty_field = Ty_Field(fields->head->name, field_type);
                
                if (tail) {
                    tail->tail = Ty_FieldList(ty_field, NULL);
                    tail = tail->tail;
                } else {
                    ty_fields = Ty_FieldList(ty_field, NULL);
                    tail = ty_fields;
                }
            }
            
            return Ty_Record(ty_fields);
        }
        
        case A_arrayTy: {
            Ty_ty array_type = S_look(tenv, a->u.array);
            if (array_type)
                return Ty_Array(array_type);
            else {
                EM_error(a->pos, "undefined type %s", S_name(a->u.array));
                return Ty_Array(Ty_Int());
            }
        }
    }
    
    assert(0);
    return NULL;
}