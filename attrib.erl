-module(attrib).
-export([load/0,init/1,new/1,set/3,get/2,add/3,sub/3,dumpstring/2,dump/1,roll/1]).

load()->
	attrib_nif:load().

init(AttribFile) when is_atom(AttribFile)->
	attrib_nif:init(AttribFile).

new(Index) when is_integer(Index)->
	attrib_nif:new(Index).

set(Attrib,Name,Value) when is_atom(Name), is_number(Value)->
	attrib_nif:set(Attrib, Name, float(Value)).

get(Attrib,Name) when is_atom(Name)->
	attrib_nif:get(Attrib, Name).

add(Attrib,Name,Value) when is_atom(Name), is_number(Value)->
	attrib_nif:add(Attrib, Name, float(Value)).

sub(Attrib,Name,Value) when is_atom(Name), is_number(Value)->
	attrib_nif:sub(Attrib, Name, float(Value)).

dumpstring(Attrib,Index) when is_integer(Index)->
	attrib_nif:dumpstring(Attrib, Index).

%% 让公式集里面的随机数重新生成新的随机数
roll(Attrib)->
    attrib_nif:roll(Attrib).

%% dump 出公式里面所有值
dump(Attrib)->
    attrib_nif:dump(Attrib).
