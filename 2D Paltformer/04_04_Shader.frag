//
// ================================================
// | Grafica pe calculator                        |
// ================================================
// | Laboratorul IV - 04_04_Shader.frag |
// ======================================
// 
//  Shaderul de fragment / Fragment shader - afecteaza culoarea pixelilor;
//

#version 330 core

//	Variabile de intrare (dinspre Shader.vert);
in vec4 ex_Color;
in vec2 tex_Coord;		//	Coordonata de texturare;

//	Variabile de iesire	(spre programul principal);
out vec4 out_Color;		//	Culoarea actualizata;

//  Variabile uniforme;
uniform sampler2D platformTexture;
uniform sampler2D playerTexture;

uniform int isPlatform;


void main(void)
  {
  
      vec4 texColor = (isPlatform == 1)
            ? texture(platformTexture, tex_Coord)
            : texture(playerTexture, tex_Coord);

        // Use texture color directly (preserve its alpha)
        out_Color = texColor;
  }