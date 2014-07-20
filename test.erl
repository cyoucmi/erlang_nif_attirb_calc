-module(test).
-export([test/0]).


test()->
     attrib:load(),
     attrib:init('./attrib.txt'),
     A = attrib:new(2),
     attrib:set(A,'力量',100),
     attrib:set(A,'敏捷',55),
     attrib:set(A,'智力',70),
     attrib:set(A,'对方的护甲',1000),
     attrib:get(A,'攻击的结果').

