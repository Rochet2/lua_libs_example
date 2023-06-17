#define SOL_SOL_ALL_SAFETIES_ON 1

#include <dirent.h>
#include "sol.h"

#include "LuaVal.h"

int main() {
	try {
		sol::state state;
		state.open_libraries();

		LuaValBase::registerMetatables(state);
		// LuaVal<false>::registerMetatables(state);

		// Could use dirent here to find lua files from a directory tree to execute

		state.script("print('1hello from lua!!')");
		// state.script("print(require('LuaVal'), 'a')");
		state.script("print(LuaVal)");
		state.script("print(LuaVal)");
		state.script("print(LuaVal)");
		state.script("LVMT = LuaVal");
		state.script("lv = LVMT.newLocked({})");
		state.script("print('2hello from lua!')");
		state.script("lv.foo = 5");
		state.script("print('3hello from lua!')");
		state.script("lv.bar = 'baz'");
		state.script("print('4hello from lua!')");
		state.script("lv.qux = {}");
		state.script("print('5hello from lua!')");
		state.script("ov = LVMT.asLua(lv)");
		state.script("print('6hello from lua!')");
		state.script("print('here', ov, ov.foo, ov.bar, ov.qux)");
		state.script("print('7hello from lua!')");
		state.script("for k,v in LVMT.iterate(lv) do print(k,v) end");
		state.script("aa, bb = LVMT.iterate(lv)");
		state.script("cc, dd = LVMT.iterate(lv)");
		state.script("local ee, ff = LVMT.iterate(lv)");
		// state.script("lv.xx = 5");
		state.script("for k,v in cc, dd do print(k,v) end");
		state.script("for k,v in aa, bb do print(k,v) end");
		state.script("print('8hello from lua!')");
		state.script("print('here', lv, lv.foo, lv.bar, lv.qux)");
		state.script("print('9hello from lua!')");
		state.script("print('here', lv.qux)");
		state.script("print('10hello from lua!')");
		state.script("print(LVMT.newLocked({ a = { b = { c = 666 } } }).a.b.c)");
		state.script("print(LVMT.new({ a = { b = { c = 777 } } }).a.b.c)");
		state.script("print(LVMT.new({}).iterate)");
		state.script("print(LVMT.new({ iterate = 5 }).iterate)");
		state.script("b = LVMT.new({ x = 5 }); b.y = LVMT.newLocked({ y = 10 }); print(b.x, b.y, b.y.y)");
		state.script("collectgarbage('step')");
		state.script("collectgarbage('step')");
		state.script("collectgarbage('step')");
	}
	catch (std::exception* ex) {
		std::cout << ex->what() << std::endl;
	}

	return 0;
}
