
%token NUMBER

%%

list: expr '\n' 
	| list expr '\n' ;

expr: NUMBER
	| expr '+' expr
	| expr '-' expr
	| expr '*' expr
	| expr '/' expr
	| '(' expr ')';

%%
