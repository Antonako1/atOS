const luajs = require('luajs');

let lua = new luajs.LuaState();


luaRunMainFun = function(cmd){
    let fileName = cmd
    return luaVM.execute(luaCode);
}
module.exports = {
    luaRunMainFun: luaRunMainFun
}