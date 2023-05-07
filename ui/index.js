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
    });
});