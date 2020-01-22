// sous mac
// g++ -I/usr/local/include/ -lglfw -lGLEW -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs IGSD.cpp -framework OpenGL -oIGSD
// ./IGSD

// sous linux
// g++ -I/usr/local/include/ -I/public/ig/glm/ -c IGSD.cpp  -IGSD.o
// g++ -I/usr/local IGSD.o -lglfw  -lGLEW  -lGL -lopencv_core -lopencv_imgproc -lopencv_highgui  -lopencv_imgcodecs -oIGSD
// ./IGSD

// Inclut les en-têtes standards
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;


/*** Déclaration des tableaux et des constantes ***/

const int tailleTab = 393600;
GLfloat g_vertex_buffer_data[tailleTab];  // tableau qui contiendra les points
GLfloat g_vertex_color_data[tailleTab];   // tableau qui contiendra les couleurs
GLfloat texCoord[(tailleTab*2)/3];   // tableau qui contiendra les textures
GLfloat gainLoss[tailleTab];     // tableau de gain et perte
GLfloat gaLoss = 0.;
GLfloat g[tailleTab/20];    // tableau qui contiendra les données d'une seule equipe
GLfloat c[tailleTab/20];    // tableau qui contiendra les couleurs attribuées aux données d'une seule equipe
GLfloat gl[tailleTab/20];   // tableau qui contiendra les gains/perte d'une seule equipe
GLfloat t[((tailleTab*2)/3)/20]; // tableau qui contiendra la texture d'une seule quipe

// LA fenetre
GLFWwindow* window;
GLuint teamIDs[20];    // tableau qui contiendra les IDs des équipes
int IDsHW[20][2];     // tableau qui contiendra les coordonnées de la texture u et v

// les angles 
float angleX = M_PI/2;
float angleY = 0.f;
float angleZ = 0.f;

int compteur = -1;

// La taille de notre fenetre
int winWidth  = 1200;
int winHeight = 800;

// La taille de notre texture
int texWidth  = -1;
int texHeight = -1;

// le vertex contenant non 20 VBOs
GLuint vertexbuffer[20];

// le vertex contenant nos VAOs
GLuint VertexArrayID[20];

// Variables d'interaction 
GLfloat ZoomIO = 0, MoveLR = 0, MoveUD = 0;

// identifiant de notre programme de shaders
GLuint programID;


// stocke les variables uniformes qui seront communes a tous les vertex dessines
GLuint MatrixID, CompteurID, uniform_texture2;

// Charge une texture et retourne l'identifiant openGL
GLuint LoadTexture(string fileName){
  GLuint tId = -1;
  // On utilise OpenCV pour charger l'image
  Mat image = imread(fileName, CV_LOAD_IMAGE_UNCHANGED);

  // On va utiliser des TEXTURE_RECTANGLE au lieu de classiques TEXTURE_2D
  // car avec ca les coordonnees de texture s'exprime en pixels  et non en coordoonnes homogenes (0.0...1.0)
  // En effet la texture est composee de lettres et symbole que nous voudrons extraire... or la position et
  // taille de ces symboles dans la texture sont connuees en "pixels". Ca sera donc plus facile

  //comme d'hab on fait generer un numero unique(ID) par OpenGL
  glGenTextures(1, &tId);

  texWidth  = image.cols;
  texHeight = image.rows;

  glBindTexture(GL_TEXTURE_RECTANGLE, tId);
    // on envoie les pixels a la carte graphique
	  glTexImage2D(GL_TEXTURE_RECTANGLE,
	  	           0,           // mipmap level => Attention pas de mipmap dans les textures rectangle
	  	           GL_RGBA,     // internal color format
	  	           image.cols,
	  	           image.rows,
	  	           0,           // border width in pixels
	  	           GL_BGRA,     // input file format. Arg le png code les canaux dans l'autre sens
	  	           GL_UNSIGNED_BYTE, // image data type
	  	           image.ptr());
	  // On peut donner des indication a opengl sur comment la texture sera utilisee
	  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // INTERDIT sur les textures rectangle!
	//glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);

  return tId;
}


/**
 Fonction qui charges les tableaux de points, rangs, IDs 
 @param fn : le fichier où se trouve les données
 @param pts : le vecteur où on stocke les points des equipes
 @param ranks : le vecteur où on stocke les rangs des equipes
 @param IDs : le tableau où on stocke les IDs de chaque équipe
 **/
void loadData(string fn, vector<int> &pts, vector<int> &ranks, GLuint IDs[], int HW[20][2]){
	ifstream file ( fn );
	string ligne="";
	int i = 0, itterator = -1;
	string mot;
	while(getline ( file, ligne, '\n' )){ // on parcourt ligne par ligne
		i=0; itterator++;
		istringstream flux(ligne);
		while(getline(flux, mot, ',')){ //on parcourt mot par mot
		    if(i==0){  // on est dans la colonne où se trouve le nom des équipes
				IDs[itterator] = LoadTexture(mot + ".png");
				HW[itterator][0] = texWidth;
				HW[itterator][1] = texHeight;
				cout << "Tex " << itterator << " : {" << IDsHW[itterator][0] << ", " << IDsHW[itterator][1] << "}" << endl;
			}
			if(i%6==1){  // colonne des rangs
				ranks.push_back(stoi(mot));
			}
			if(i%6==2){  // colonne des points
				pts.push_back(stoi(mot));
			}
			i++;
		}

	}
}

// Charge un programme de shaders, le compile et recupere dedans des pointeurs vers
// les variables homogenes que nous voudront mettre a jour plus tard, a chaque dessin
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if(VertexShaderStream.is_open()){
		string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if(FragmentShaderStream.is_open()){
		string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}


	// Link the program
	printf("Linking program\n");
	GLuint progID = glCreateProgram();
	glAttachShader(progID, VertexShaderID);
	glAttachShader(progID, FragmentShaderID);
	glLinkProgram(progID);


	// Check the program
	glGetProgramiv(progID, GL_LINK_STATUS, &Result);
	glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(progID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(progID, VertexShaderID);
	glDetachShader(progID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return progID;
}


GLFWwindow *initMainwindow(){
  // Nous allons apprendre a lire une texture de "symboles" generee a partir d'un outil comme :
	// https://evanw.github.io/font-texture-generator/

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // On veut OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Pour rendre MacOS heureux ; ne devrait pas être nécessaire
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // On ne veut pas l'ancien OpenGL
  glfwWindowHint(GLFW_DEPTH_BITS, 24);

	// Ouvre une fenêtre et crée son contexte OpenGl
	GLFWwindow *win = glfwCreateWindow( winWidth, winHeight, "Main 06", NULL, NULL);
	if( win == NULL ){
	    fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are maybe not 3.3 compatible. \n" );
	    glfwTerminate();
	}

  //
	glfwMakeContextCurrent(win);

	// Assure que l'on peut capturer la touche d'échappement
	glfwSetInputMode(win, GLFW_STICKY_KEYS, GL_TRUE);

	// active une callback = une fonction appellee automatiquement quand un evenement arrive
	/*glfwSetKeyCallback(win, key_callback);*/
	return win;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // on teste si la touche E est pressee et si c'est le cas on re-genere des donnees
    if (key== GLFW_KEY_SPACE && action == GLFW_PRESS){
      compteur=-1;
    } else if (key== GLFW_KEY_H && action == GLFW_PRESS ){
      (compteur<=0)?compteur = 0 : compteur--;
    } else if (key== GLFW_KEY_B && action == GLFW_PRESS){
      (compteur>=19)?compteur = 19 : compteur++;
    }

    if(glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS){
      angleX+=M_PI/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_RIGHT ) == GLFW_PRESS){
      angleX-=M_PI/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_UP ) == GLFW_PRESS){
      angleY+=M_PI/150;
    }
    if(glfwGetKey(window, GLFW_KEY_DOWN ) == GLFW_PRESS){
      angleY-=M_PI/150;
    }

    if(glfwGetKey(window, GLFW_KEY_A ) == GLFW_PRESS){ // Q
      MoveLR-=1/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_D ) == GLFW_PRESS){ // Dœ
      MoveLR+=1/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_W ) == GLFW_PRESS){ // Z Up
      MoveUD+=1/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_S ) == GLFW_PRESS){ // S Down
      MoveUD-=1/150.;
    }

    if(glfwGetKey(window, GLFW_KEY_Q ) == GLFW_PRESS){ // A Zoom In
      ZoomIO+=1/150.;
    }
    if(glfwGetKey(window, GLFW_KEY_E ) == GLFW_PRESS){ // E Zoom Out
      ZoomIO-=1/150.;
    }
}



int main(){
  // Initialise GLFW
	if( !glfwInit() ) {
	    fprintf( stderr, "Failed to initialize GLFW\n" );
	    return -1;
	}

  window = initMainwindow();

  // Initialise GLEW
	glewExperimental=true; // Nécessaire dans le profil de base
	if (glewInit() != GLEW_OK) {
	    fprintf(stderr, "Failed to initialize GLEW\n");
	    return -1;
	}
  // Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	glDepthRange(-1, 1);


    GLfloat l = 0.05; // Rayon de la courbe d'une équipe
	GLfloat l2 = 0.05; // Rayon du cylindre
	string path = "rankspts.csv"; // notre fichier
	vector<int> points,classements; // les vecteurs qui vont contenir les points et rangs de nos données
	loadData(path,points,classements,teamIDs, IDsHW); // on fait appelle à loadData pour charger nos données dans les vecteurs déclarés
	GLuint nbrPtsPerimetre = 40;  // nombre de points sur les cercles des demi cylindres
	for (int equipe = 0; equipe < 20; equipe++)
	{
		// Notre tableau de couleurs
		GLfloat color[3];
					if(equipe<4){GLfloat color[3]={21/255., 96/255., 189/255.};} // Bleu
					else { if(equipe<7){GLfloat color[3]={205/255., 215/255., 0/255.};} // Jaune
						   else { if(equipe<10){GLfloat color[3]={188./255, 191./255, 212./255};} // Gris clair
						   	      else { if(equipe<16) {GLfloat color[3]={101./255, 101./255, 101./255};} // Gris foncé
						   	      		 else {GLfloat color[3]={238/255., 16/255., 16/255.};} // Rouge
						   	    	   }
						        }
						 }

		for (int journees = 0; journees < 40; journees++){

			gaLoss = 0.f;
			GLfloat xcA = ((journees%41)/41.)*2 -1;
			GLfloat xcB = ((journees%41)/41. + 1/55.)*2 - 1;
			GLfloat xcASuivant = (((journees+1)%41)/41.)*2 -1;   // *2 : pour avoir des plus grandes cases, - 1 c'est pas le rendre visinle dans notre plan
			GLfloat dy = (( (19-classements[(equipe*41+journees)])/19. + (points[(equipe*41+journees)])/98. ) +( (19-classements[(equipe*41+journees)])/19. + (points[(equipe*41+journees)])/98. ) + l)/2. -1;
			GLfloat dySuivant = (( (19-classements[equipe*41+journees+1])/19. + (points[equipe*41+journees+1])/98. ) +( (19-classements[equipe*41+journees+1])/19. + (points[equipe*41+journees+1])/98. ) + l)/2. -1;
            GLfloat zc = 0.f;
			l2 = 0.05f;

			for (int i = 0; i <= nbrPtsPerimetre; i++)
			{
				/** demi cylindre droit **/
				// nos données (sommets à tracer)
				g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6] = xcA;
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 1] = dy + l/2.*cos(i*M_PI/nbrPtsPerimetre);
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 2] = zc + l2/2.*sin(i*M_PI/nbrPtsPerimetre);

				g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 3] = xcB;
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 4] = dy + l/2.*cos(i*M_PI/nbrPtsPerimetre);
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 5] = zc + l2/2.*sin(i*M_PI/nbrPtsPerimetre);

				gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6] = 0;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 1] =0;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 2] = 0;

				gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 3] = 0;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 4] =0;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 5] = 0;
				GLfloat ombre = (i*(0.30/nbrPtsPerimetre))-0.30;

					// les couleurs de nos sommets
				g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6] = color[0]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 1] = color[1]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 2] = color[2]-ombre;

				g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 3] = color[0]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 4] = color[1]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + i*6 + 5] = color[2]-ombre;


					// la position de nos textures 
			  texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + i*4] = ((journees%5)*IDsHW[equipe][0])/5.;
				texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + i*4 + 1] = (i*IDsHW[equipe][1])/(1.*nbrPtsPerimetre);

			  texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + i*4 + 2] = (((journees+1)%5)*IDsHW[equipe][0])/5;
				texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + i*4 + 3] = (i*IDsHW[equipe][1])/(1.*nbrPtsPerimetre);

			}
			    /** Demi cylindre penché **/
				// on commencera par(nbrPtsPerimetre*6+6) = (nbrPtsPerimetre+1)*6
			if(dySuivant>dy) gaLoss = 2;
			for (int i = 0; i <= nbrPtsPerimetre; i++)
			{
				// les sommets à tracer
				g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6] = xcB;
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 1] = dy + l/2.*cos((nbrPtsPerimetre-i)*M_PI/nbrPtsPerimetre);
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 2] = zc + l2/2.*sin((nbrPtsPerimetre-i)*M_PI/nbrPtsPerimetre);

				g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 3] = xcASuivant;
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 4] = dySuivant + l/2.*cos((nbrPtsPerimetre-i)*M_PI/nbrPtsPerimetre);
					g_vertex_buffer_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 5] = zc + l2/2.*sin((nbrPtsPerimetre-i)*M_PI/nbrPtsPerimetre);

				gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6] = gaLoss;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 1] = gaLoss;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 2] = gaLoss;

				gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 3] = gaLoss;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 4] = gaLoss;
					gainLoss[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 5] = gaLoss;

				GLfloat ombre = -(i*(0.30/nbrPtsPerimetre)); // Car on les remplit à l'envers !

					// les couleurs de nos sommets 
				g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6] = color[0]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 1] = color[1]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 2] = color[2]-ombre;

				g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 3] = color[0]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 4] = color[1]-ombre;
					g_vertex_color_data[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*6 + (nbrPtsPerimetre+1)*6 + i*6 + 5] = color[2]-ombre;


					// position de nos textures
				texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + (nbrPtsPerimetre+1)*4 + i*4] = (((journees+1)%5)*IDsHW[equipe][0])/5;
				  texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + (nbrPtsPerimetre+1)*4 + i*4 + 1] =(i*IDsHW[equipe][1])/(1.*nbrPtsPerimetre);

				texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + (nbrPtsPerimetre+1)*4 + i*4 + 2] = (((journees+2)%5)*IDsHW[equipe][0])/5;
				  texCoord[(equipe*40+journees)*2*(nbrPtsPerimetre+1)*4 + (nbrPtsPerimetre+1)*4 + i*4 + 3] = (i*IDsHW[equipe][1])/(1.*nbrPtsPerimetre);
			}
		

		}
	}



    // On construit un VAO pour chaque equipe (on crée nos 20 VAOs)
	for(int i=0; i<20; i++){
		glGenVertexArrays(1, &VertexArrayID[i]);
	}
	
	// On construit un VAB pour chaque equipe (on crée nos 20 VBOs)
	for(int i=0; i<20; i++){
		glGenBuffers(1, &vertexbuffer[i]);
	}

	// On construit pour chaque equipe son tableau de données, couleurs des données et gain/perte
	for (int i = 0; i < 20; i++)
	{
		glBindVertexArray(VertexArrayID[i]);
			for(int j = 0; j < tailleTab/20 ; j++){
				// On coupe notre long tableau de données, couleurs gains/pertes par 20 (et donc par équipe) avant de les envoyer au VAO
				g[j] = g_vertex_buffer_data[i*(tailleTab/20) + j];
				c[j] = g_vertex_color_data[i*(tailleTab/20) + j];
				gl[j] = gainLoss[i*(tailleTab/20) + j];
			}

      for(int j = 0; j < ((tailleTab*2)/3)/20 ; j++){
        t[j] = texCoord[i*(((tailleTab*2)/3)/20) + j];
      }
			// The following commands will talk about our 'vertexbuffer' buffer
			  glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[i]);
			  // Only allocqte memory. Do not send yet our vertices to OpenGL.
			  glBufferData(GL_ARRAY_BUFFER, sizeof(g)+sizeof(c)+sizeof(gl)+sizeof(t), 0, GL_STATIC_DRAW);

		      // send vertices in the first part of the buffer
			  glBufferSubData(GL_ARRAY_BUFFER, 0,                            sizeof(g), g);

			  // send colors in the second part of the buffer
			  glBufferSubData(GL_ARRAY_BUFFER, sizeof(g), sizeof(c), c);

				// send gaLOss in the third part of the buffer
			  glBufferSubData(GL_ARRAY_BUFFER, sizeof(g)+sizeof(c), sizeof(gl), gl);
             glBufferSubData(GL_ARRAY_BUFFER, sizeof(g)+sizeof(c)+sizeof(gl), sizeof(t),t);

				// ici les commandes stockees "une fois pour toute" dans le VAO
				glVertexAttribPointer(
				   0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				   3,                  // size
				   GL_FLOAT,           // type
				   GL_FALSE,           // normalized?
				   0,                  // stride
				   (void*)0            // array buffer offset
				);
				glEnableVertexAttribArray(0);

		    glVertexAttribPointer( 
		    	1,
		    	3,
		    	GL_FLOAT,
		    	GL_FALSE,
		    	0,
		    	(void*)sizeof(g)
				);
				glEnableVertexAttribArray(1);

				glVertexAttribPointer( 
		    	2,
		    	3,
		    	GL_FLOAT,
		    	GL_FALSE,
		    	0,
		    	(void*)(sizeof(g)+sizeof(c))
				);
				glEnableVertexAttribArray(2);
 
			   glVertexAttribPointer( 
		    		3,
		    		2,		// Car coordonnées de textures 2D = UV
		    		GL_FLOAT,
		    		GL_FALSE,
		    		0,
		    		(void*)(sizeof(g)+sizeof(c)+sizeof(gl))
					);
					glEnableVertexAttribArray(3);

				glBindBuffer(GL_ARRAY_BUFFER, 0);

			// on desactive le VAO a la fin de l'initialisation
			glBindVertexArray (0);

	}



	  // Assure que l'on peut capturer la touche d'échappement enfoncée ci-dessous
		glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	  glfwSetKeyCallback(window, key_callback);
		
		// On lie nos shaders avec notre code opengl
		programID = LoadShaders( "SimpleVertexShader62.vertexshader", "SimpleFragmentShader62.fragmentshader" );
		MatrixID = glGetUniformLocation(programID, "MVP");
	    CompteurID = glGetUniformLocation(programID, "COMPTEUR");
	    uniform_texture2 = glGetUniformLocation(programID, "loctexture");
		
		GLfloat co = 0.f;
		GLfloat com = 0.f;

	do{
		// clear before every draw 1
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.02f, 0.2f, 1.0f);
    	// Use our shader program
		glm::mat4 projectionMatrix = glm::perspective(45.0f, 1024.0f / 768.0f, 0.1f, 200.0f);
		glm::mat4 viewMatrix       = glm::lookAt(
			                      vec3(MoveLR+2*cos(angleX),MoveUD+2*sin(angleY),ZoomIO+2*sin(angleX)), // where is the camara

			                      vec3(MoveLR,MoveUD,0), //where it looks
			                      vec3(0,1,0) // head is up
			                    );
		glm::mat4 modelMatrix      = glm::mat4(1.0);


		glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
        glUseProgram(programID);
      	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
        for(int i=0; i<20; i++){
          glUniform1i(uniform_texture2, 0);

  			if(compteur == -1 ){
  				glUniform1i(CompteurID, 0);
  			} else {
  				if(compteur == i) {
  					glUniform1i(CompteurID, 1);
  				} else {
  					glUniform1i(CompteurID, 2);
  				}
  			}
      		// on re-active le VAO avant d'envoyer les buffers
  			glBindVertexArray(VertexArrayID[i]);

			glActiveTexture(GL_TEXTURE0);

			glBindTexture(GL_TEXTURE_RECTANGLE, teamIDs[i]);
  				// Draw the triangle(s)41
  				glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(g)/(3*sizeof(float))); // Starting from vertex 0 .. all the buffer

			glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  				// on desactive le VAO a la fin du dessin
  				glBindVertexArray (0);

  		}

        
		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Vérifie si on a appuyé sur la touche échap (ESC) ou si la fenêtre a été fermée
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
}
