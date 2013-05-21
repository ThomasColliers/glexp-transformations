$(init);

function valueChanged(){
    var form = $(this).parents('form:first');
    // build matrix array
    var matrix = new Array(16);
    for(var column = 0; column < 4; column++){
        for(var row = 0; row < 4; row++){
            var rowElement = $('.row',form).eq(row);
            var val = $('input',rowElement).eq(column).val();
            matrix[column*4+row] = parseFloat(val,10);
        }
    }
    matrixUpdate(JSON.stringify({"matrix":matrix}));
}

function init(){
    // add event listener to each 
    $('form input').on('keyup',valueChanged).on('change',valueChanged).on('click',valueChanged);
}
