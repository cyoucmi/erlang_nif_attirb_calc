-module(attrib_nif).
-export([load/0,init/1,new/1,set/3,get/2,add/3,sub/3,dumpstring/2,dump/1,roll/1]).

load()->
	erlang:load_nif("./attrib_nif", 0).

init(_)->
	"attrib_nif library not loaded".

new(_)->
	"attrib_nif library not loaded".

set(_,_,_)->
	"attrib_nif library not loaded".

get(_,_)->
	"attrib_nif library not loaded".

add(_,_,_)->
	"attrib_nif library not loaded".

sub(_,_,_)->
	"attrib_nif library not loaded".

dumpstring(_,_)->
	"attrib_nif library not loaded".

roll(_)->
	"attrib_nif library not loaded".

dump(_)->
	"attrib_nif library not loaded".

