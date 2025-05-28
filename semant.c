#include <stdio.h>
#include <stdlib.h>
#include "semant.h"
#include "env.h"
#include <stddef.h> 
#include <assert.h>
#include "errormsg.h"
#include "prabsyn.h"


void SEM_transProg(A_exp exp) {
    S_table tenv = E_base_tenv();
    S_table venv = E_base_venv();
    
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
    if (!v) {
        EM_error(0, "internal error: NULL variable");
        return expTy(NULL, Ty_Int());
    }

    switch(v->kind) {
        case A_simpleVar: {
            //fprintf(stderr,"started simple var\n");
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry){
                //fprintf(stderr,"simple var returned\n");
                return expTy(NULL, actual_ty(x->u.var.ty));
            } else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }
        }
        
        case A_fieldVar: {
            //fprintf(stderr,"started field var\n");
            struct expty var = transVar(venv, tenv, v->u.field.var);
            if (var.ty->kind != Ty_record) {
                EM_error(v->pos, "not a record type");
                return expTy(NULL, Ty_Int());
            }
            
            Ty_fieldList fields = var.ty->u.record;
            while (fields) {
                if (fields->head->name == v->u.field.sym) {
                    return expTy(NULL, actual_ty(fields->head->ty));
                }
                fields = fields->tail;
            }
            
            EM_error(v->pos, "field %s not found", S_name(v->u.field.sym));
            return expTy(NULL, Ty_Int());
        }
        
        case A_subscriptVar: {
            //fprintf(stderr,"started subscript var\n");
            struct expty var = transVar(venv, tenv, v->u.subscript.var);
            if (var.ty->kind != Ty_array) {
                EM_error(v->pos, "array type required");
                return expTy(NULL, Ty_Int());
            }
            
            struct expty index = transExp(venv, tenv, v->u.subscript.exp);
            if (index.ty->kind != Ty_int) {
                EM_error(v->pos, "integer required for array index");
            }
            
            return expTy(NULL, actual_ty(var.ty->u.array));
        }
        
        default:
            EM_error(v->pos, "unknown variable type");
            return expTy(NULL, Ty_Int());
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
            //fprintf(stderr,"started call exp\n");
            E_enventry func = S_look(venv, a->u.call.func);
            //fprintf(stderr,"1\n");
            if (!func || func->kind != E_funEntry) {
                EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Int());
            }
            //fprintf(stderr,"2\n");
            
            // pr_exp(stderr, a, 0);
            
            A_expList args = a->u.call.args;
            Ty_tyList formals = func->u.fun.formals;
            //fprintf(stderr,"3\n");
            while (args && formals) {
                struct expty arg = transExp(venv, tenv, args->head);

                //fprintf(stderr,"5\n");

                Ty_ty formal_actual = actual_ty(formals->head);
                Ty_ty arg_actual = actual_ty(arg.ty);
                
                //fprintf(stderr, "Argument evaluated to type kind: %d\n", arg.ty->kind);
                //fprintf(stderr, "Expected formal parameter type kind: %d\n", formals->head->kind);
                //fprintf(stderr, "After resolution - Arg: %d, Formal: %d\n", arg_actual->kind, formal_actual->kind);
                
                if (formal_actual->kind != arg_actual->kind) {
                    EM_error(args->head->pos, "argument type mismatch");
                }
                //fprintf(stderr,"6\n");
                args = args->tail;
                formals = formals->tail;
            }
            //fprintf(stderr,"4\n");

            if (args) {
                EM_error(a->pos, "too many arguments");
            } else if (formals) {
                EM_error(a->pos, "too few arguments");
            }

            //fprintf(stderr, "Function %s result type: %p\n", S_name(a->u.call.func), func->u.fun.result);
            if (func->u.fun.result) {
                //fprintf(stderr, "Result type kind: %d\n", func->u.fun.result->kind);
            } else {
                //fprintf(stderr, "ERROR: Result type is NULL\n");
            }
            //fprintf(stderr,"end call exp\n");
            
            return expTy(NULL, func->u.fun.result);
        }
        
        case A_opExp: {
            //fprintf(stderr,"started op exp\n");
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

            } else if (oper == A_eqOp || oper == A_neqOp) {
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

            } else if (oper == A_ltOp || oper == A_leOp || 
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
            } else {
                EM_error(a->pos, "unknown operator");
                return expTy(NULL, Ty_Int());
            }
            break;
        }
        case A_recordExp: {
            // fprintf(stderr,"started record exp\n");
            Ty_ty record_ty = S_look(tenv, a->u.record.typ);
            if (!record_ty) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Int());
            }
            
            record_ty = actual_ty(record_ty);
            
            if (record_ty->kind != Ty_record) {
                EM_error(a->pos, "%s is not a record type", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            }
            
            A_efieldList fields = a->u.record.fields;
            Ty_fieldList ty_fields = record_ty->u.record;
            
            while (fields && ty_fields) {
                Ty_fieldList tf = ty_fields;
                bool found = false;

                
                
                while (tf && !found) {
                    if (fields->head->name == tf->head->name) {
                        struct expty field_exp = transExp(venv, tenv, fields->head->exp);

                        Ty_ty field_exp_actual = actual_ty(field_exp.ty); 
                        Ty_ty tf_actual = actual_ty(tf->head->ty);


                        // fprintf(stderr, "Argument evaluated to type kind: %d\n", field_exp.ty->kind);
                        // fprintf(stderr, "Expected formal parameter type kind: %d\n", tf->head->ty->kind);
                        // fprintf(stderr, "After resolution - Arg: %d, Formal: %d\n", field_exp_actual->kind, tf_actual->kind);
                                    
                        if (field_exp_actual->kind != tf_actual->kind) {
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
            //fprintf(stderr,"started seq exp\n");
            A_expList seq = a->u.seq;
            struct expty result = expTy(NULL, Ty_Void());
            
            while (seq) {
                result = transExp(venv, tenv, seq->head);
                //fprintf(stderr, "Processing seq expression kind: %d\n", seq->head->kind);
                if (seq->head->kind == A_assignExp) {
                    //fprintf(stderr, "Processing assignment in sequence\n");
                    if (seq->head->u.assign.var->kind == A_simpleVar) {
                        S_symbol var_name = seq->head->u.assign.var->u.simple;
                        E_enventry entry = S_look(venv, var_name);
                        //fprintf(stderr, "Variable %s in sequence: %p\n", S_name(var_name), entry);
                    }
                }
                seq = seq->tail;
            }
            
            return result;
        }
        
        case A_assignExp: {
            //fprintf(stderr,"started assign exp\n");
            struct expty var = transVar(venv, tenv, a->u.assign.var);
            struct expty exp = transExp(venv, tenv, a->u.assign.exp);
            
            if (var.ty->kind != exp.ty->kind && 
                !(var.ty->kind == Ty_record && exp.ty->kind == Ty_nil)) {
                EM_error(a->pos, "type mismatch in assignment");
            }
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_ifExp: {
            //fprintf(stderr,"started if exp\n");
            struct expty test = transExp(venv, tenv, a->u.iff.test);
            struct expty then = transExp(venv, tenv, a->u.iff.then);
            
            if (test.ty->kind != Ty_int) {
                EM_error(a->u.iff.test->pos, "integer (boolean) required for condition");
            }
            
            if (a->u.iff.elsee) {
                struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
                
                return expTy(NULL, then.ty);
            } else {
                return expTy(NULL, Ty_Void());
            }
        }
        
        case A_whileExp: {
            //fprintf(stderr,"started while exp\n");
            struct expty test = transExp(venv, tenv, a->u.whilee.test);
            struct expty body = transExp(venv, tenv, a->u.whilee.body);
            
            if (test.ty->kind != Ty_int) {
                EM_error(a->u.whilee.test->pos, "integer (boolean) required for condition");
            }
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_forExp: {
            //fprintf(stderr,"started for exp\n");
            struct expty lo = transExp(venv, tenv, a->u.forr.lo);
            struct expty hi = transExp(venv, tenv, a->u.forr.hi);
            
            if (lo.ty->kind != Ty_int) {
                EM_error(a->u.forr.lo->pos, "integer required for lower bound");
            }
            if (hi.ty->kind != Ty_int) {
                EM_error(a->u.forr.hi->pos, "integer required for upper bound");
            }
            
            S_beginScope(venv);
            S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Int()));
            
            transExp(venv, tenv, a->u.forr.body);
            
            S_endScope(venv);
            
            return expTy(NULL, Ty_Void());
        }
        
        case A_breakExp:
            return expTy(NULL, Ty_Void());
            
        case A_letExp: {
            // //fprintf(stderr,"started let exp\n");
            // pr_exp(stderr, a, 0);
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
            // fprintf(stderr,"started array exp\n");
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
            
            struct expty size = transExp(venv, tenv, a->u.array.size);
            if (size.ty->kind != Ty_int) {
                EM_error(a->u.array.size->pos, "integer required for array size");
            }
            
            struct expty init = transExp(venv, tenv, a->u.array.init);

            Ty_ty init_actual = actual_ty(init.ty);
            Ty_ty array_actual = actual_ty(array_ty->u.array);

            // fprintf(stderr, "Argument evaluated to type kind: %d\n", init.ty->kind);
            // fprintf(stderr, "Expected formal parameter type kind: %d\n", array_ty->u.array->kind);
            // fprintf(stderr, "After resolution - Arg: %d, Formal: %d\n", init_actual->kind, array_actual->kind);
            

            if (init_actual->kind != array_actual->kind) {
                EM_error(a->u.array.init->pos, "type mismatch in array initializer");
            }
            
            return expTy(NULL, array_ty);
        }
    }

    return expTy(NULL, Ty_Void());
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    
    switch(d->kind) {
        case A_varDec: {
            //fprintf(stderr,"started var dec...\n");
            struct expty e = transExp(venv, tenv, d->u.var.init);
            S_enter(venv, d->u.var.var, E_VarEntry(e.ty));
            //fprintf(stderr,"end var dec\n");
            return;
        }
        case A_typeDec: {
            //fprintf(stderr,"started type dec...\n");
            A_nametyList types;
    
            for (types = d->u.type; types; types = types->tail) {
                S_enter(tenv, types->head->name, Ty_Name(types->head->name, NULL));
            }
            
            for (types = d->u.type; types; types = types->tail) {
                Ty_ty type = transTy(tenv, types->head->ty);
                Ty_ty name_ty = S_look(tenv, types->head->name);
                if (name_ty && name_ty->kind == Ty_name) {
                    name_ty->u.name.ty = type;
                }
            }
            //fprintf(stderr,"end type dec\n");
            return;
        }
        case A_functionDec: {
            //fprintf(stderr,"started function dec...\n");
            // pr_dec(stderr, d, 0);
            A_fundec f = d->u.function->head;
            Ty_ty resultTy;
            if(f -> result){
                resultTy = S_look(tenv, f->result);
                if (!resultTy) {
                    // Debug the missing type
                    //fprintf(stderr, "ERROR: Return type %s not found for function %s\n", S_name(f->result), S_name(f->name));
                    // Provide a fallback type instead of NULL
                    // resultTy = Ty_Int();
                } else {
                    //fprintf(stderr, "DEBUG: Return type %s  for function %s\n", S_name(f->result), S_name(f->name));
                }
                
            } else
                resultTy = Ty_Void();
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
            //fprintf(stderr,"end function dec\n");
            return;
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
                return Ty_Int();
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
    
    return NULL;
}