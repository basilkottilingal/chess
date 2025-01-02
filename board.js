let board = [
  ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],
  ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],
  ['', '', '', '', '', '', '', ''],
  ['', '', '', '', '', '', '', ''],
  ['', '', '', '', '', '', '', ''],
  ['', '', '', '', '', '', '', ''],
  ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],
  ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'],
];

// JavaScript to create an 8x8 grid using a for loop
const gridContainer = document.getElementById('grid-container');

// Loop through 64 times (8 rows * 8 columns = 64 grid items)
for (let i = 0; i < 8; i++) {
  for (let j = 0; j < 8; j++) {
    
    const gridItem = document.createElement('div');

    gridItem.classList.add('grid-item');
    
    gridItem.id = String.fromCharCode('a'.charCodeAt(0) + j) + (8 - i);
    
    //square id of chess board.
    if( (i%2)^(j%2) ){
      gridItem.style.backgroundColor = "#bbbbbb";
    }
  
    // Create an image element
    if (board[i][j] != '') {
      const img = document.createElement('img');
      img.src = "chesspieces/" + board[i][j] + ".png"; 
      img.alt = board[i][j];
      img.id = "piece";
      /* Only the chess pieces are movable */
      img.draggable = "true";
      // Append the image to the grid item
      gridItem.appendChild(img);
    }

    // Append the grid item to the grid container
    gridContainer.appendChild(gridItem);
  }
}
