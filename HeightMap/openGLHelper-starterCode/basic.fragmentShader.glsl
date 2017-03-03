#version 150

in vec4 col;
out vec4 c;

uniform vec4 color;

void main()
{
  // compute the final pixel color
    c=col;
  // c = vec4(1,1,1,1);
}

