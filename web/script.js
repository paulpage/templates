// Canvas DOM object and drawing context
var c, ctx;

let grid = {
    width: 30,
    height: 20,
    data: null,
    cellSize: null,
}

let colors = {
    background: 'black',
    foreground: 'white',
    grid: 'gray',
    text: 'black',
};

/**
 * Automatically set the optimum canvas size based on window width and height
 */
function setCanvasSize() {
    ratio = grid.width / grid.height;
    if (window.innerWidth / grid.width < window.innerHeight / grid.height) {
        c.width = window.innerWidth * 0.8;
        c.height = c.width / grid.width * grid.height;
    } else {
        c.height = window.innerHeight * 0.8;
        c.width = c.height / grid.height * grid.width;
    }
    grid.cellSize = c.width / grid.width;
}

window.onload = function() {

    // Populate grid with 0s
    numCells = grid.width * grid.height;
    grid.data = Array.apply(null, Array(numCells)).map(Number.prototype.valueOf, 0);

    document.addEventListener('keydown', function(event) {
        switch (event.keyCode) {
        }
    });

    document.addEventListener('click', function(event) {
        index = coordsToIndex(mouseToGrid(getMousePos(event)));
        // grid.data[index] = (grid.data[index] == 1 ? 0 : 1);
        grid.data[index] += 1;
    });

    c = document.getElementById('canvas');
    ctx = c.getContext('2d');

    setCanvasSize();

    window.addEventListener("resize", setCanvasSize, false);

    requestAnimationFrame(draw);
}

/**
 * Draw the grid.
 * Runs once every frame.
 */
function draw() {
    ctx.fillStyle = colors.background;
    ctx.fillRect(0, 0, c.width, c.height);

    ctx.strokeStyle = colors.grid;
    ctx.beginPath();
    for (var x = 0; x < grid.width; x++) {
        ctx.moveTo(x * grid.cellSize, 0);
        ctx.lineTo(x * grid.cellSize, c.height);
    }
    for (var y = 0; y < grid.height; y++) {
        ctx.moveTo(0, y * grid.cellSize);
        ctx.lineTo(c.width, y * grid.cellSize);
    }
    ctx.stroke();

    for (var i = 0; i < grid.data.length; i++) {
        drawCell(i, grid.data[i]);
    }
    requestAnimationFrame(draw);
}

/**
 * Draw an individual cell
 */
function drawCell(pos, val) {
    if (val > 0) {

        var x = pos % grid.width;
        var y = (pos - pos % grid.width) / grid.width;

        var piece_x = x * grid.cellSize;
        var piece_y = y * grid.cellSize;

        ctx.fillStyle = colors.foreground;
        ctx.fillRect(piece_x, piece_y, grid.cellSize, grid.cellSize);

        var text_x = piece_x + (grid.cellSize / 2);
        var text_y = piece_y + (grid.cellSize / 2);
        ctx.font = Math.floor(grid.cellSize * 0.8) + "px Sans";
        ctx.textAlign = "center";
        ctx.textBaseline = "middle";
        ctx.fillStyle = colors.text;
        ctx.fillText(Math.pow(2, val), text_x, text_y);
    }
}

function mouseToGrid(pos) {
    return {
        x: Math.floor(pos.x / grid.cellSize),
        y: Math.floor(pos.y / grid.cellSize)
    };
}

function getMousePos(evt) {
    var rect = c.getBoundingClientRect();
    return {
        x: evt.clientX - rect.left,
        y: evt.clientY - rect.top
    };
}

/**
 * Translates x and y coordinates to a grid array index
 */
function coordsToIndex(pos) {
    return pos.y * grid.width + pos.x;
}
