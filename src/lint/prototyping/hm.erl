-module(hm).
-export([ex/0, hm1/0]).

-include("lint.hrl").

%% foo f g x = if f(x == 1) then g(x) else 20
ex() ->
    Equations = [{int, int},
                 {3, int},
                 {6, bool},
                 {1, {[6], 5}},
                 {2, {[3], 7}},
                 {5, bool},
                 {4, 7},
                 {4, int},
                 {0, {[1, 2, 3], 4}}],
    Node = #node{},
    Substitutions = unify_all_equations(Equations, Node, maps:new()),
    {[{[bool],bool},{[int],int},int],int} =
        dereference(Substitutions, 0).

%% fn foo(f, g, x) {
%%     if f(x ==Int= 1) {
%%         g(x)
%%     } else {
%%         20
%%     }
%% }
hm1() ->
    {Source, Result, Node, AdornedEquations} =
        lint:start("../../../examples/sa/hm1.sa"),
    io:format("==== Source:\n~s\n", [Source]),
    io:format("==== Lint output:\n~s\n", [Result]),
    %%io:format("==== Node:\n~p\n\n", [Node]),
    %%io:format("==== Adorned equations:\n~p\n\n", [AdornedEquations]),
    Equations =
        lists:map(fun(#equation{type = Type}) ->  Type end, AdornedEquations),
    %%io:format("==== Equations:\n~p\n", [Equations]),
    io:format("==== Pretty printed equations:\n"),
    lists:foreach(
      fun(#equation{type = {Left, Right}}) ->
              io:format("~s -> ~s\n",
                        [type_to_string(Left), type_to_string(Right)])
      end, AdornedEquations),
    io:format("\n"),
    case unify_all_equations(Equations, Node, maps:new()) of
        {mismatch, TypeStack} ->
            {mismatch, TypeStack};
        Substitutions ->
            %%io:format("==== Substitutions:\n~p\n", [Substitutions]),
            io:format("==== Solution:\nt0 -> ~s\n",
                      [type_to_string(dereference(Substitutions, 0))])
    end.

%%
%% Unify all equations
%%

unify_all_equations([], _Node, Substitutions) ->
    Substitutions;
unify_all_equations([{Left, Right}|Rest], Node, Substitutions) ->
    case unify(Left, Right, [{Left, Right}], Node, Substitutions) of
        {mismatch, TypeStack} ->
            {mismatch, TypeStack};
        UpdatedSubstitutions ->
            unify_all_equations(Rest, Node, UpdatedSubstitutions)
    end.

unify(_X, _Y, _TypeStack, _Node, {mismatch, TypeStack}) ->
    {mismatch, TypeStack};
unify(X, X, _TypeStack, _Node, Substitutions) ->
    Substitutions;
unify(X, Y, TypeStack, Node, Substitutions) when is_integer(X) ->
    unify_variable(X, Y, TypeStack, Node, Substitutions);
unify(X, Y, TypeStack, Node, Substitutions) when is_integer(Y) ->
    unify_variable(Y, X, TypeStack, Node, Substitutions);
unify({ArgsX, ReturnX}, {ArgsY, ReturnY}, TypeStack, Node, Substitutions) ->
    case length(ArgsX) /= length(ArgsY) of
        true ->
            {mismatch, TypeStack};
        false ->
            unify_args(ArgsX, ArgsY, TypeStack, Node,
                       unify(ReturnX, ReturnY, [{ReturnX, ReturnY}|TypeStack],
                             Node, Substitutions))
    end;
unify(_, _, TypeStack, _, _) ->
    {mismatch, TypeStack}.

unify_args([], [], _TypeStack, _MetInfo, Substitutions) ->
    Substitutions;
unify_args([X|Xs], [Y|Ys], TypeStack, Node, Substitutions) ->
    unify_args(Xs, Ys, TypeStack, Node,
               unify(X, Y, [{X, Y}|TypeStack], Node, Substitutions)).

unify_variable(Variable, Type, TypeStack, Node, Substitutions)
  when is_integer(Variable) ->
    case maps:get(Variable, Substitutions, undefined) of
        undefined when is_integer(Type) ->
            case maps:get(Type, Substitutions, undefined) of
                undefined ->
                    case occurs_check(Variable, Type, Substitutions) of
                        true ->
                            {mismatch, TypeStack};
                        false ->
                            Substitutions#{Variable => Type}
                    end;
                Substitution ->
                    unify(Variable, Substitution,
                          [{Variable, Substitution}|TypeStack], Node,
                          Substitutions)

            end;
        undefined ->
            case occurs_check(Variable, Type, Substitutions) of
                true ->
                    {mismatch, TypeStack};
                false ->
                    Substitutions#{Variable => Type}
            end;
        Substitution ->
            unify(Substitution, Type, [{Substitution, Type}|TypeStack],
                  Node, Substitutions)
    end.

occurs_check(Variable, Variable, _Substitutions) when is_integer(Variable) ->
    true;
occurs_check(Variable, Type, Substitutions)
  when is_integer(Type) andalso is_integer(Variable) ->
    case maps:get(Type, Substitutions, undefined) of
        undefined ->
            case Type of
                {ArgTypes, ReturnType} ->
                    occurs_check(Variable, ReturnType, Substitutions) orelse
                        lists:any(
                          fun(ArgType) ->
                                  occurs_check(ArgType, ArgType, Substitutions)
                          end, ArgTypes);
                _ ->
                    false
            end;
        Substitution ->
            occurs_check(Variable, Substitution, Substitutions)
    end;
occurs_check(_Variable, _Type, _Substitutions) ->
    false.

%% error_message(_Node, TypeStack) ->
%%     io_lib:format("Type mismatch: ~p\n", [TypeStack]).

%% error_message(Node, []) ->
%%     [];
%% error_message(Node, [{X, Y}|Rest]) ->
%%     [type_to_string(Node, X), "->", type_to_string(Node, Y), "\n",
%%      error_message(Node, Rest)].

%% type_to_string(_Node, int) ->
%%     "int";x ==Int= 1
%% type_to_string(_Node, bool) ->
%%     "bool";
%% type_to_string(Node, TypeVariable) when is_integer(TypeVariable) ->
%%     case search_by_type(Node, TypeVariable) of
%%         #node{row = Row, value = undefined} ->
%%             ["t", integer_to_list(TypeVariable), ":", integer_to_list(Row),
%%              "::"];
%%         #node{row = Row, value = Value} ->
%%             ["t", integer_to_list(TypeVariable), ":", integer_to_list(Row),
%%              ":", Value, ":"]
%%     end;
%% type_to_string(Node, {ArgTypes, ReturnType}) ->
%%     "(" ++ type_to_string(Node, ArgTypes) ++ ") -> " ++
%%         type_to_string(Node, ReturnType);
%% type_to_string(Node, []) ->
%%     [];
%% type_to_string(Node, [Type]) ->
%%     type_to_string(Node, Type);
%% type_to_string(Node, [Type|Types]) ->
%%     type_to_string(Node, Type) ++ ", " ++ type_to_string(Node, Types).

%% search_by_type(Node, Type) when Node#node.type =:= Type ->
%%     Node;
%% search_by_type(Node, Type) ->
%%     search_children(Node#node.children, Type).

%% search_children([], _Type) ->
%%     undefined;
%% search_children([Child|Rest], Type) ->
%%     case search_by_type(Child, Type) of
%%         undefined ->
%%             search_children(Rest, Type);
%%         Node ->
%%             Node
%%     end.

%%
%% Dereference
%%

dereference(_Substitutions, BaseType) when is_atom(BaseType) ->
    BaseType;
dereference(Substitutions, Variable) when is_integer(Variable) ->
    dereference(Substitutions, maps:get(Variable, Substitutions));
dereference(Substitutions, {ArgTypes, ReturnType}) ->
    {lists:map(fun(Type) -> dereference(Substitutions, Type) end, ArgTypes),
     dereference(Substitutions, ReturnType)}.

%%
%% Pretty print type
%%

type_to_string(int) ->
    "int";
type_to_string(bool) ->
    "bool";
type_to_string(TypeVariable) when is_integer(TypeVariable) ->
    "t" ++ integer_to_list(TypeVariable);
type_to_string({[ArgType], ReturnType}) ->
    "(" ++ type_to_string(ArgType) ++ " -> " ++
        type_to_string(ReturnType) ++ ")";
type_to_string({ArgTypes, ReturnType}) ->
    "(("++ type_to_string(ArgTypes) ++ ") -> " ++
        type_to_string(ReturnType) ++ ")";
type_to_string([]) ->
    "";
type_to_string([Type]) ->
    type_to_string(Type);
type_to_string([Type|Rest]) ->
    type_to_string(Type) ++ ", " ++ type_to_string(Rest).
