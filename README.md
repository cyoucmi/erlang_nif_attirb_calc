erlang_nif_attirb_calc
======================

A game attribute calculation implementation in C and write a driver by erlang nif

Before you use, You need change $ERL_ROOT in makefile

You can use like:

1> attrib:load().%%load Nif module
ok

2> attrib:init('./attrib.txt'). %%init by attribute formula file
ok

3> A = attrib:new(1). %% New a attrib by formula group index
<<>>

4> attrib:set(A,strength,10). %% set value
ok

5> attrib:set(A,ming_jie,2).  %% set value
ok

6> attrib:set(A,zhi_li,0.6).    %% set value
ok

7> attrib:get(A,attack_result). %% get value
102.30000305175781

8> attrib:dump(A). %% Dump  all value (for debug)
dump values = zhi_li : 0.600000attack_result : 102.300003 hu_jia : 0.000000 attack : 102.300003 strength : 10.000000 bi_li : 0.000000  ming_jie : 2.000000

9> attrib:dumpstring(A,1).%% dump the formula use suffix notation 
DUMPSTRING = attack  strength  10  * ming_jie  + zhi_li 0.5 * + = 
