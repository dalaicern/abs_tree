a.out: parsetest.o y.tab.o lex.yy.o errormsg.o util.o absyn.o symbol.o table.o semant.o types.o env.o prabsyn.o
	cc -g parsetest.o y.tab.o lex.yy.o errormsg.o util.o absyn.o symbol.o table.o semant.o types.o env.o prabsyn.o

parsetest.o: parsetest.c errormsg.h util.h
	cc -g -c parsetest.c

y.tab.o: y.tab.c
	cc -g -c y.tab.c

y.tab.c: tiger.grm
	yacc -dv tiger.grm

y.tab.h: y.tab.c
	echo "y.tab.h was created at the same time as y.tab.c"

errormsg.o: errormsg.c errormsg.h util.h
	cc -g -c errormsg.c

lex.yy.o: lex.yy.c y.tab.h errormsg.h util.h
	cc -g -c lex.yy.c

#lex.yy.c: tiger.lex
#	lex tiger.lex

prabsyn.o: prabsyn.c prabsyn.h util.h symbol.h absyn.h
	cc -g -c prabsyn.c

absyn.o: absyn.c absyn.h util.h symbol.h
	cc -g -c absyn.c

symbol.o: symbol.c symbol.h util.h
	cc -g -c symbol.c

table.o: table.c table.h util.h
	cc -g -c table.c

env.o: env.c env.h util.h
	cc -g -c env.c

semant.o: semant.c semant.h util.h
	cc -g -c semant.c

types.o: types.c types.h util.h
	cc -g -c types.c

util.o: util.c util.h
	cc -g -c util.c

clean: 
	rm -f a.out *.o
