

 const socket = new WebSocket("ws://localhost:8080"); // Change as needed

  
socket.onopen = function () {
      console.log("Connected to wsServer");
  };

  socket.onmessage = function (event) {
      console.log("Received:", event.data);
  };

  socket.onclose = function () {
      console.log("Connection closed");
  };
  function sendMessage() {
      socket.send("Hello Server!");
  }

    // Wait for the DOM to load before executing the function
    
document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
  const button = document.getElementById('restart');
      // Add the event listener for the click event
  button.addEventListener('click', function() {
    //alert('Button clicked!');
    socket.send("Restart");
  });
});
    
document.addEventListener('DOMContentLoaded', function() {
      // Get the button element by ID
  const button = document.getElementById('handshake');
      // Add the event listener for the click event
  button.addEventListener('click', function() {
    //alert('Button clicked!');
    socket.send("Hello Server!");
  });
});

  
/*
// Add an event listener for the 'input' event
document.getElementById("fen").addEventListener("blur", function(event) {
  // Update the output element with the value of the input field
  document.getElementById("fenValidity").textContent = "FEN: " + event.target.value;

});
*/

const inputElement = document.getElementById("fen");
const outputElement = document.getElementById("fenValidity");
const submitButton = document.getElementById("fenEnter");


let debounceTimer;

// Function to handle the result
function sendFen() {
  socket.send(inputElement.value);
  //if invalid
    inputElement.textContent = "";
  inputElement.disabled = true;
  submitButton.disabled = true;
}

// Handle the input event with debounce
inputElement.addEventListener("input", function() {
  clearTimeout(debounceTimer); // Clear the previous timer
  debounceTimer = setTimeout(function() {
  }, 500); // Adjust 500ms as needed
});

// Trigger on Enter key press
inputElement.addEventListener("keydown", function(event) {
  if (event.key === "Enter") {
    //event.preventDefault(); // Prevent form submission
    sendFen();
  }
});

// Trigger on button click
submitButton.addEventListener("click", function() {
  sendFen();
});
