
var maze = {};
var size;
var originBFS = [0,0];
var color = "white";
var h;
var w;
var pos,dir;
var loop ;
var step;


function setup() {

  createCanvas(windowWidth,windowHeight);
  size =windowHeight/80;
  loop = true;
  step = 0;
  h = (windowHeight-10)/size|0;
  w = (windowWidth-10)/size|0;
  h--;
  w--;




  for (var i = h; i >= 0; i--) {
      for (var j = w; j >= 0; j--) {
        maze[i.toString()+","+j.toString()] = 0;
      }
  }

  pos = createVector(w/2|0,h/2|0);

  dir = createVector(-1,0);



}

function mousePressed()
{
  loop = !loop;
}
function draw() {
  if(loop){



    if(step==0)
    {
      var x = originBFS[0];
      var y = originBFS[1];

      for (var i = 0; i < h+1 ; i++) {
        for (var j = 0; j < w+1; j++) {
          fill("white");
          rect(x,y,size,size)
          x= x+size;
        }
        y+=size ;
        x = originBFS[0];
      }
    }

    if(maze[pos.y.toString()+","+pos.x.toString()]==0)
    {
      maze[pos.y.toString()+","+pos.x.toString()]=1;
      fill("black");
      rect(pos.x*size,pos.y*size,size,size)
      dir = left(dir);
      pos = createVector(pos.x+dir.x,pos.y+dir.y);
      fill("green");
      rect(pos.x*size,pos.y*size,size,size)

    }
    else
    {
      if(maze[pos.y.toString()+","+pos.x.toString()]==1)
      {
       maze[pos.y.toString()+","+pos.x.toString()]=0;
        fill("white");
        rect(pos.x*size,pos.y*size,size,size)
        dir = right (dir);
        pos = createVector(pos.x+dir.x,pos.y+dir.y);

        fill("green");
        rect(pos.x*size,pos.y*size,size,size)

      }
    }


    if(pos.x >   w)
    {
      noLoop();
    }
    if(pos.x<0)
    {
      noLoop();
    }

    if(pos.y > h)
    {
      noLoop();
    }
    if(pos.y<0)
    {
      noLoop();
    }
  step++;








  }

}

function right(dir)
{
  if(dir.x == 1 && dir.y == 0)
  {
    return createVector(0,-1);
  }
  if(dir.x == 0 && dir.y == 1)
  {
    return createVector(1,0);
  }
  if(dir.x == -1 && dir.y == 0)
  {
    return createVector(0,1);
  }
  if(dir.x == 0 && dir.y == -1)
  {
    return createVector(-1,0);
  }
}
function left(dir)
{
  if(dir.x == 1 && dir.y == 0)
  {
    return createVector(0,1);
  }
  if(dir.x == 0 && dir.y == 1)
  {
    return createVector(-1,0);
  }
  if(dir.x == -1 && dir.y == 0)
  {
    return createVector(0,-1);
  }
  if(dir.x == 0 && dir.y == -1)
  {
    return createVector(1,0);
  }
}
