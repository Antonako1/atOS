// Add new case to cmdlets.js
// And this
/*
const example = require("./example.js");
const helloWorld = example.helloWorld;
*/
// Make function
helloWorld = function(){
    return "| Hello world!"
}
// Export functions
module.exports = {
    helloWorld: helloWorld
};