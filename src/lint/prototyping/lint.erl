-module(lint).
-compile(export_all).

-include("lint.hrl").

start() ->
    start("../../../bin/lint", "../../../examples/sa/hm1.sa").

start(Filename) ->
    start("../../../bin/lint", Filename).

start(Lint, Filename) ->
    Source = os:cmd("cat -n " ++ Filename),
    case os:cmd(Lint ++ " < " ++ Filename) of
        "Error: " ++ Reason ->
            {error, Reason};
        RawLintOutput ->
            Lines = string:split(RawLintOutput, "\n", all),
            {Tree, Equations} =
                lists:splitwith(fun(Line) -> Line /= "" end, Lines),
            {[Node], []} = parse_tree(Tree, 0),
            SortedEquations =
                lists:sort(fun(#equation{user_defined = X},
                               #equation{user_defined = Y}) ->
                                   X > Y
                           end, parse_equations(Equations)),
            {Source, RawLintOutput, Node, SortedEquations}
    end.

%%
%% Parse tree
%%

parse_tree([], _Level) ->
    {[], []};
parse_tree([""|Rest], Level) ->
    parse_tree(Rest, Level);
parse_tree([Line|Rest] = Lines, Level) ->
    case parse_line(Line) of
        {Name, Row, Value, Type, Level} ->
            {Children, SiblingLines} = parse_tree(Rest, Level + 1),
            {Siblings, RestLines} = parse_tree(SiblingLines, Level),
            {[#node{name = Name,
                    row = list_to_integer(Row),
                    value = Value,
                    type = parse_node_type(string:trim(Type)),
                    children = Children}|Siblings],
             RestLines};
        _ ->
            {[], Lines}
    end.

parse_node_type("") ->
    undefined;
parse_node_type(Type) ->
    parse_type(Type).

parse_line(Line) ->
    [Name, Row, Value, Type] = string:split(string:trim(Line), ":", all),
    {Name, Row, Value, Type, calc_indent(Line)}.

calc_indent(Line) ->
    calc_indent(Line, 0) div 2.

calc_indent([$ |Rest], N) ->
    calc_indent(Rest, N + 1);
calc_indent(_, N) ->
    N.

%%
%% Parse equations
%%

parse_equations([]) ->
    [];
parse_equations([""|Rest]) ->
    parse_equations(Rest);
parse_equations([Line|Rest]) ->
    [OriginNodeName, OriginRow, Row, UserDefined, Value, Type] =
        string:split(Line, ":", all),
    [#equation{origin_node_name = OriginNodeName,
               origin_row = list_to_integer(OriginRow),
               row = list_to_integer(Row),
               value = Value,
               type = parse_type(Type),
               user_defined = parse_is_user_defined(UserDefined)}|
     parse_equations(Rest)].

parse_type(Line) ->
    {ok, Tokens, _} = erl_scan:string(Line ++ "."),
    {ok, Parsed} = erl_parse:parse_exprs(Tokens),
    [Expr] = Parsed,
    {value, Result, _} = erl_eval:exprs([Expr], erl_eval:new_bindings()),
    Result.

parse_is_user_defined("0") ->
    false;
parse_is_user_defined("1") ->
    true.
