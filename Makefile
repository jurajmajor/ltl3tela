#    Copyright (c) 2016 Juraj Major
#
#    This file is part of LTL3TELA.
#
#    LTL3TELA is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    LTL3TELA is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with LTL3TELA.  If not, see <http://www.gnu.org/licenses/>.

FILES = alternating.cpp nondeterministic.cpp automaton.cpp utils.cpp spotela.cpp main.cpp

ltl3tela: $(FILES)
	g++ -std=c++17 -O2 -o ltl3tela $(FILES) -lspot -lbddx

clean:
	rm ltl3tela
