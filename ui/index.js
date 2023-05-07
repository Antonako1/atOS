// Make the background square pattern
let teksti = "";
let t = "";
for (let i = 0; i < 16; i++) {
    t += "<tr>";
    for (let j = 0; j < 16; j++) {
        teksti = '<td class="iconArea" id="'+i+"."+j+'"></td>'; 
        t += teksti;
    }
    t += "</tr>";
}
document.getElementById("backgroundMain").innerHTML = t;

// Event listener for the td icon presses
// Get all the <td> elements with the class "iconArea"
const tdElements = document.querySelectorAll('.iconArea');

// Iterate over the <td> elements and attach an event listener
tdElements.forEach((td) => {
    td.addEventListener('click', function() {
        // Handle the click event here
        console.log('Clicked on element with ID:', this.id);

        // Loop through file.json to find if it has items in it
        for (let i in location.filedata) {
            let item = location.filedata[i];
            if (item && item.name == name) {
                location.filedata.splice(i, 1);
                return name + " was deleted with success";
            }
        }
        return "No such file found"
    });
});

const findItemMain = require('../src/findItem.js');
// Find buttons event listener
const findButton = document.getElementById('findButton');
findButton.addEventListener('click', function() {
  let txtData = document.getElementById('textFind').value;
  console.log('txtData::: ', txtData);

  let result = findItemMain(txtData);
  console.log('result::: ', result);
  document.getElementById('findResultString').innerHTML = result;
});
