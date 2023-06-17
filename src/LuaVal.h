// BSD-3-Clause Copyright (c) 2022, Rochet2 <rochet2@post.com> All rights
// reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// * Neither the name of the copyright holder nor the names of its contributors
//   may be used to endorse or promote products derived from this software
//   without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <memory> // std::unique_ptr
#include <mutex> // std::mutex, std::unique_lock
#include <shared_mutex> // std::shared_mutex, std::unique_lock
#include <utility> // std::tuple

extern "C"
{
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
};

enum class LOCK_STATUS {
    LOCKED,
    NOT_LOCKED,
};

class LuaValBase
{
public:
    struct MapLess {
        bool operator()(const std::unique_ptr<LuaValBase>& a, const std::unique_ptr<LuaValBase>& b) const {
            return a->lessThan(*b);
        }
    };
    struct MapEq {
        bool operator()(const std::unique_ptr<LuaValBase>& a, const std::unique_ptr<LuaValBase>& b) const {
            if (typeid(*a) != typeid(*b)) {
                return false;
            }
            return a->equalTo(*b);
        }
    };
    struct MapHash {
        std::size_t operator()(const std::unique_ptr<LuaValBase>& k) const {
            return k->LuaValHash();
        }
    };
    typedef std::unordered_map<std::unique_ptr<LuaValBase>, std::unique_ptr<LuaValBase>, MapHash, MapEq> MapType;
    typedef std::tuple<typename MapType::iterator, typename MapType::iterator> IteratorState;
    typedef std::tuple<typename MapType::iterator, typename MapType::iterator, std::shared_lock<std::shared_mutex>> IteratorStateLocked;

    static constexpr const char* LUAVAL_METATABLE_KEY = "LuaVal";
    static constexpr const char* LUAVAL_ITERATOR_METATABLE_KEY = "LuaVal Iterator Metatable";
    static constexpr const char* LUAVAL_LOCKED_ITERATOR_METATABLE_KEY = "Locked LuaVal Iterator Metatable";

    virtual ~LuaValBase() {
        // Required by abstract base class
    }

    // Helpers

    int abs_index(lua_State* L, int i) {
        return i > 0 || i <= LUA_REGISTRYINDEX ? i : lua_gettop(L) + i+1;
    }

    template<typename T>
    static int pushLuaVal(lua_State* L, T* v, const char* metatable)
    {
        if (!v) {
            lua_pushnil(L);
            return 1;
        }
        T** ptr = (T**)lua_newuserdata(L, sizeof(T*));
        *ptr = v;
        luaL_getmetatable(L, metatable);
        lua_setmetatable(L, -2);
        return 1;
    }

    static bool isLuaVal(lua_State* L, int index, const char* metatable)
    {
        if (!lua_isuserdata(L, index))
            return false;
        if (!lua_getmetatable(L, index))
            return false;
        luaL_getmetatable(L, metatable);
        bool result = lua_rawequal(L, -1, -2) != 0;
        lua_pop(L, 2);
        return result;
    }

    template<typename T>
    static T* getLuaVal(lua_State* L, int index)
    {
        return *(T**)lua_touserdata(L, index);
    }

    template<typename T>
    static T* checkLuaVal(lua_State* L, int index, const char* metatable)
    {
        return *(T**)luaL_checkudata(L, index, metatable);
    }

    static std::unique_ptr<LuaValBase> AsLuaVal(lua_State* L, int index, LOCK_STATUS status);

    virtual bool lessThan(const LuaValBase& other) const = 0;
    virtual bool equalTo(const LuaValBase& other) const = 0;
    virtual int pushAsLua(lua_State* L, uint32_t depth) = 0;
    virtual size_t LuaValHash() const = 0;
    virtual int asObject(lua_State* L) = 0;
    virtual std::unique_ptr<LuaValBase> clone() = 0;
    virtual int Get(lua_State* L, int self_index, int key_index) {
        return luaL_argerror(L, self_index, "Trying to use non table value as table");
    }
    virtual int Set(lua_State* L, int self_index, int key_index, int val_index) {
        return luaL_argerror(L, self_index, "Trying to use non table value as table");
    }
    virtual int iterate(lua_State* L, int self_index) {
        return luaL_argerror(L, self_index, "Trying to use non table value as table");
    }

    // Lua functions

    template<typename T>
    static int gc_closure(lua_State* L)
    {
        delete getLuaVal<T>(L, 1);
        return 0;
    }

    static int factory(lua_State* L)
    {
        std::unique_ptr<LuaValBase> v = AsLuaVal(L, 1, LOCK_STATUS::NOT_LOCKED);
        return pushLuaVal(L, v.release(), LUAVAL_METATABLE_KEY);
    }

    static int factoryLocked(lua_State* L)
    {
        std::unique_ptr<LuaValBase> v = AsLuaVal(L, 1, LOCK_STATUS::LOCKED);
        return pushLuaVal(L, v.release(), LUAVAL_METATABLE_KEY);
    }

    static int Get(lua_State* L) {
        constexpr int self_index = 1;
        constexpr int key_index = 2;
        LuaValBase* luaval = checkLuaVal<LuaValBase>(L, self_index, LUAVAL_METATABLE_KEY);
        return luaval->Get(L, self_index, key_index);
    }

    static int Set(lua_State* L) {
        constexpr int self_index = 1;
        constexpr int key_index = 2;
        constexpr int val_index = 3;
        LuaValBase* luaval = checkLuaVal<LuaValBase>(L, self_index, LUAVAL_METATABLE_KEY);
        return luaval->Set(L, self_index, key_index, val_index);
    }

    static int iterate(lua_State* L)
    {
        constexpr int self_index = 1;
        LuaValBase* luaval = checkLuaVal<LuaValBase>(L, self_index, LUAVAL_METATABLE_KEY);
        return luaval->iterate(L, self_index);
    }

    static int pushAsLua(lua_State* L)
    {
        LuaValBase* luaval = checkLuaVal<LuaValBase>(L, 1, LUAVAL_METATABLE_KEY);
        int depth = static_cast<int>(luaL_optinteger(L, 2, 0));
        return luaval->pushAsLua(L, depth);
    }

    static void registerMetatables(lua_State* L)
    {
        if (luaL_newmetatable(L, LUAVAL_METATABLE_KEY) == 0)
        {
            lua_pop(L, 1);
            luaL_error(L, "Metatable %s already registered", LUAVAL_METATABLE_KEY);
            return;
        }
        lua_pushstring(L, "__gc");
        lua_pushcclosure(L, &gc_closure<LuaValBase>, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "asLua");
        lua_pushcclosure(L, &pushAsLua, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "new");
        lua_pushcclosure(L, &factory, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "newLocked");
        lua_pushcclosure(L, &factoryLocked, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "iterate");
        lua_pushcclosure(L, &iterate, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__index");
        lua_pushcclosure(L, &Get, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__newindex");
        lua_pushcclosure(L, &Set, 0);
        lua_rawset(L, -3);

        // Set LuaVal as a global variable
        lua_setglobal(L, LUAVAL_METATABLE_KEY); // lua_pop(L, 1);

        if (luaL_newmetatable(L, LUAVAL_ITERATOR_METATABLE_KEY) == 0)
        {
            lua_pop(L, 1);
            luaL_error(L, "Metatable %s already registered", LUAVAL_ITERATOR_METATABLE_KEY);
            return;
        }
        lua_pushstring(L, "__gc");
        lua_pushcclosure(L, &gc_closure<IteratorState>, 0);
        lua_rawset(L, -3);
        lua_pop(L, 1);

        if (luaL_newmetatable(L, LUAVAL_LOCKED_ITERATOR_METATABLE_KEY) == 0)
        {
            lua_pop(L, 1);
            luaL_error(L, "Metatable %s already registered", LUAVAL_LOCKED_ITERATOR_METATABLE_KEY);
            return;
        }
        lua_pushstring(L, "__gc");
        lua_pushcclosure(L, &gc_closure<IteratorStateLocked>, 0);
        lua_rawset(L, -3);
        lua_pop(L, 1);
    }
};

template<typename T>
class LuaVal : public LuaValBase
{
protected:
    T v;
public:

    LuaVal(const T& v) : v(v) {}

    size_t LuaValHash() const override
    {
        return std::hash<decltype(v)>{}(v);
    }

    static int asObject(lua_State* L, double value)
    {
        lua_pushnumber(L, value);
        return 1;
    }
    static int asObject(lua_State* L, bool value)
    {
        lua_pushboolean(L, value);
        return 1;
    }
    static int asObject(lua_State* L, std::string const& value)
    {
        lua_pushlstring(L, value.c_str(), value.size());
        return 1;
    }
    int asObject(lua_State* L) override
    {
        return asObject(L, v);
    }

    bool lessThan(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return true;
        }
        return v < static_cast<const LuaVal<T>&>(other).v;
    }
    bool equalTo(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return false;
        }
        return v == static_cast<const LuaVal<T>&>(other).v;
    }

    int pushAsLua(lua_State* L, uint32_t depth) override
    {
        return asObject(L);
    }

    std::unique_ptr<LuaValBase> clone() override {
        return std::make_unique<LuaVal<T>>(v);
    }
};
template class LuaVal<double>;
template class LuaVal<bool>;
template class LuaVal<std::string>;

class LuaValTable;
class LuaValTableLocked;

class LuaValTable : public LuaValBase
{
protected:
    MapType v;
public:
    LuaValTable() : LuaValBase(), v() {
    }
    LuaValTable(LuaValTable& lv) : LuaValBase(), v() {
        for (auto& it : lv.v)
        {
            v[it.first->clone()] = it.second->clone();
        }
    }
    friend class LuaValTableLocked;
    LuaValTable(LuaValTableLocked& lv);

    int Get(lua_State* L, int self_index, int key_index) override {
        auto klv = AsLuaVal(L, key_index, LOCK_STATUS::NOT_LOCKED);
        if (!klv)
            return luaL_argerror(L, key_index, "Table key is nil");
        auto it = v.find(klv);
        if (it == v.end())
        {
            lua_pushnil(L);
            return 1;
        }
        else
        {
            auto& val = it->second;
            return val->asObject(L);
        }
    }

    int Set(lua_State* L, int self_index, int key_index, int val_index) override {
        auto kk = AsLuaVal(L, key_index, LOCK_STATUS::NOT_LOCKED);
        auto vv = AsLuaVal(L, val_index, LOCK_STATUS::NOT_LOCKED);
        if (!kk)
            return luaL_argerror(L, key_index, "Table key is nil");
        if (!vv)
            v.erase(kk);
        else
            v.insert_or_assign(std::move(kk), std::move(vv));
        return 0;
    }

    static int iterate_closure(lua_State* L)
    {
        if (!isLuaVal(L, 1, LUAVAL_ITERATOR_METATABLE_KEY)) {
            return luaL_argerror(L, 1, "Trying to iterate using invalid iterator object");
        }
        auto state_tuple = getLuaVal<IteratorState>(L, 1);
        if (std::get<0>(*state_tuple) == std::get<1>(*state_tuple))
        {
            // iteration ended
            return 0;
        }
        auto oldit = std::get<0>(*state_tuple)++;
        return oldit->first->asObject(L) + oldit->second->asObject(L);
    }

    int iterate(lua_State* L, int self_index) override
    {
        lua_pushcfunction(L, &iterate_closure);
        return 1 + pushLuaVal(L, new IteratorState(v.begin(), v.end()), LUAVAL_ITERATOR_METATABLE_KEY);
    }

    int pushAsLua(lua_State* L, uint32_t depth) override
    {
        lua_newtable(L);
        for (auto& it : v) {
            if (depth == 1)
            {
                it.first->asObject(L);
                it.second->asObject(L);
                lua_rawset(L, -3);
            }
            if (depth == 0)
            {
                it.first->pushAsLua(L, depth);
                it.second->pushAsLua(L, depth);
                lua_rawset(L, -3);
            }
            else
            {
                it.first->pushAsLua(L, depth - 1);
                it.second->pushAsLua(L, depth - 1);
                lua_rawset(L, -3);
            }
        }
        return 1;
    }

    int asObject(lua_State* L) override
    {
        return pushLuaVal(L, new LuaValTable(*this), LUAVAL_METATABLE_KEY);
    }

    size_t LuaValHash() const override
    {
        return std::hash<decltype(this)>{}(this);
    }

    void FromTable(lua_State* L, int index)
    {
        int real_idex = abs_index(L, index);
        lua_pushnil(L);
        while (lua_next(L, real_idex))
        {
            auto key = AsLuaVal(L, -2, LOCK_STATUS::NOT_LOCKED);
            auto value = AsLuaVal(L, -1, LOCK_STATUS::NOT_LOCKED);
            // skip nil keys and values
            if (key && value)
                v.emplace(std::move(key), std::move(value));
            lua_pop(L, 1);
        }
    }

    bool lessThan(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return true;
        }
        return &v < &static_cast<const LuaValTable&>(other).v;
    }
    bool equalTo(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return false;
        }
        return &v == &static_cast<const LuaValTable&>(other).v;
    }

    std::unique_ptr<LuaValBase> clone() override {
        auto c = std::make_unique<LuaValTable>();
        for (const auto& k_v : v) {
            c->v[k_v.first->clone()] = k_v.second->clone();
        }
        return c;
    }
};

class LuaValTableLocked : public LuaValBase
{
protected:
    MapType v;
    std::shared_mutex lock;
public:
    LuaValTableLocked() : LuaValBase(), v() {
    }
    LuaValTableLocked(LuaValTableLocked& lv) : LuaValBase(), v() {
        std::shared_lock guard(lv.lock);
        for (auto& it : lv.v)
        {
            v[it.first->clone()] = it.second->clone();
        }
    }
    friend class LuaValTable;
    LuaValTableLocked(LuaValTable& lv);

    int Get(lua_State* L, int self_index, int key_index) override {
        auto klv = AsLuaVal(L, key_index, LOCK_STATUS::LOCKED);
        if (!klv)
            return luaL_argerror(L, key_index, "Table key is nil");
        std::shared_lock guard(lock);
        auto it = v.find(klv);
        if (it == v.end())
        {
            lua_pushnil(L);
            return 1;
        }
        else
        {
            auto& val = it->second;
            return val->asObject(L);
        }
    }

    int Set(lua_State* L, int self_index, int key_index, int val_index) override {
        auto kk = AsLuaVal(L, key_index, LOCK_STATUS::LOCKED);
        auto vv = AsLuaVal(L, val_index, LOCK_STATUS::LOCKED);
        if (!kk)
            return luaL_argerror(L, key_index, "Table key is nil");
        std::unique_lock guard(lock);
        if (!vv)
            v.erase(std::move(kk));
        else
            v.insert_or_assign(std::move(kk), std::move(vv));
        return 0;
    }

    static int iterate_closure_locked(lua_State* L)
    {
        if (!isLuaVal(L, 1, LUAVAL_LOCKED_ITERATOR_METATABLE_KEY)) {
            return luaL_argerror(L, 1, "Trying to iterate using invalid iterator object");
        }
        auto state_tuple = getLuaVal<IteratorStateLocked>(L, 1);
        if (std::get<0>(*state_tuple) == std::get<1>(*state_tuple))
        {
            // If iteration ended, free the mutex
            // We also free the mutex in __gc method of the iterator
            auto& guard = std::get<2>(*state_tuple);
            if (guard)
                guard.unlock();
            return 0;
        }
        auto oldit = std::get<0>(*state_tuple)++;
        return oldit->first->asObject(L) + oldit->second->asObject(L);
    }

    int iterate(lua_State* L, int self_index) override
    {
        lua_pushcfunction(L, &iterate_closure_locked);
        std::shared_lock guard(lock);
        return 1 + pushLuaVal(L, new IteratorStateLocked(v.begin(), v.end(), std::move(guard)), LUAVAL_LOCKED_ITERATOR_METATABLE_KEY);
    }

    int pushAsLua(lua_State* L, uint32_t depth) override
    {
        lua_newtable(L);
        std::shared_lock guard(lock);
        for (auto& it : v) {
            if (depth == 1)
            {
                it.first->asObject(L);
                it.second->asObject(L);
                lua_rawset(L, -3);
            }
            if (depth == 0)
            {
                it.first->pushAsLua(L, depth);
                it.second->pushAsLua(L, depth);
                lua_rawset(L, -3);
            }
            else
            {
                it.first->pushAsLua(L, depth - 1);
                it.second->pushAsLua(L, depth - 1);
                lua_rawset(L, -3);
            }
        }
        return 1;
    }

    int asObject(lua_State* L) override
    {
        return pushLuaVal(L, new LuaValTableLocked(*this), LUAVAL_METATABLE_KEY);
    }

    size_t LuaValHash() const override
    {
        return std::hash<decltype(this)>{}(this);
    }

    void FromTable(lua_State* L, int index)
    {
        int real_idex = abs_index(L, index);
        lua_pushnil(L);
        while (lua_next(L, real_idex))
        {
            auto key = AsLuaVal(L, -2, LOCK_STATUS::LOCKED);
            auto value = AsLuaVal(L, -1, LOCK_STATUS::LOCKED);
            // skip nil keys and values
            if (key && value)
                v.emplace(std::move(key), std::move(value));
            lua_pop(L, 1);
        }
    }

    bool lessThan(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return true;
        }
        return &v < &static_cast<const LuaValTableLocked&>(other).v;
    }
    bool equalTo(const LuaValBase& other) const override {
        if (typeid(*this) != typeid(other)) {
            return false;
        }
        return &v == &static_cast<const LuaValTableLocked&>(other).v;
    }

    std::unique_ptr<LuaValBase> clone() override {
        auto c = std::make_unique<LuaValTableLocked>();
        for (const auto& k_v : v) {
            c->v[k_v.first->clone()] = k_v.second->clone();
        }
        return c;
    }
};


LuaValTable::LuaValTable(LuaValTableLocked& lv) : LuaValBase(), v() {
    std::shared_lock guard(lv.lock);
    for (auto& it : lv.v)
    {
        v[it.first->clone()] = it.second->clone();
    }
}
LuaValTableLocked::LuaValTableLocked(LuaValTable& lv) : LuaValBase(), v() {
    for (auto& it : lv.v)
    {
        v[it.first->clone()] = it.second->clone();
    }
}

std::unique_ptr<LuaValBase> LuaValBase::AsLuaVal(lua_State* L, int index, LOCK_STATUS status)
{
    auto t = lua_type(L, index);
    switch (t)
    {
    case LUA_TBOOLEAN:
        return std::make_unique<LuaVal<bool>>(lua_toboolean(L, index) != 0);
    case LUA_TNIL:
        [[fallthrough]];
    case LUA_TNONE:
        return nullptr;
    case LUA_TNUMBER:
        return std::make_unique<LuaVal<double>>(lua_tonumber(L, index));
    case LUA_TSTRING:
    {
        size_t len;
        const char* cstr = lua_tolstring(L, index, &len);
        return std::make_unique<LuaVal<std::string>>(std::string(cstr, len));
    }
    case LUA_TTABLE:
    {
        if (status == LOCK_STATUS::LOCKED) {
            auto m = std::make_unique<LuaValTableLocked>();
            m->FromTable(L, index);
            return m;
        }
        else {
            auto m = std::make_unique<LuaValTable>();
            m->FromTable(L, index);
            return m;
        }
    }
    case LUA_TUSERDATA:
    {
        if (isLuaVal(L, index, LUAVAL_METATABLE_KEY))
        {
            LuaValBase* lv = getLuaVal<LuaValBase>(L, index);
            if (status == LOCK_STATUS::LOCKED && typeid(*lv) == typeid(LuaValTable)) {
                return std::make_unique<LuaValTableLocked>(*static_cast<LuaValTable*>(lv));
            }
            if (status == LOCK_STATUS::NOT_LOCKED && typeid(*lv) == typeid(LuaValTableLocked)) {
                return std::make_unique<LuaValTable>(*static_cast<LuaValTableLocked*>(lv));
            }
            return lv->clone();
        }
        [[fallthrough]];
    }
    default:
        luaL_argerror(L, index, "Trying to use unsupported type");
    }
    return nullptr;
}
