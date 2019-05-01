
#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#include <OpenGL/glu.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/glew.h>
#include <GL/freeglut.h>
#endif

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
const unsigned int windowWidth = 512, windowHeight = 512;

int majorVersion = 3, minorVersion = 0;

bool keyboardState[256];

void getErrorInfo(unsigned int handle)
{
    int logLen;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLen);
    if (logLen > 0)
    {
        char * log = new char[logLen];
        int written;
        glGetShaderInfoLog(handle, logLen, &written, log);
        printf("Shader log:\n%s", log);
        delete log;
    }
}

void checkShader(unsigned int shader, char * message)
{
    int OK;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &OK);
    if (!OK)
    {
        printf("%s!\n", message);
        getErrorInfo(shader);
    }
}

void checkLinking(unsigned int program)
{
    int OK;
    glGetProgramiv(program, GL_LINK_STATUS, &OK);
    if (!OK)
    {
        printf("Failed to link shader program!\n");
        getErrorInfo(program);
    }
}

// row-major matrix 4x4
struct mat4
{
    float m[4][4];
public:
    mat4() {}
    mat4(float m00, float m01, float m02, float m03,
         float m10, float m11, float m12, float m13,
         float m20, float m21, float m22, float m23,
         float m30, float m31, float m32, float m33)
    {
        m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
        m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
        m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
        m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
    }
    
    mat4 operator*(const mat4& right)
    {
        mat4 result;
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                result.m[i][j] = 0;
                for (int k = 0; k < 4; k++) result.m[i][j] += m[i][k] * right.m[k][j];
            }
        }
        return result;
    }
    operator float*() { return &m[0][0]; }
};


// 3D point in homogeneous coordinates
struct vec4
{
    float v[4];
    
    vec4(float x = 0, float y = 0, float z = 0, float w = 1)
    {
        v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    }
    
    vec4 operator*(const mat4& mat)
    {
        vec4 result;
        for (int j = 0; j < 4; j++)
        {
            result.v[j] = 0;
            for (int i = 0; i < 4; i++) result.v[j] += v[i] * mat.m[i][j];
        }
        return result;
    }
    
    vec4 operator+(const vec4& vec)
    {
        vec4 result(v[0] + vec.v[0], v[1] + vec.v[1], v[2] + vec.v[2], v[3] + vec.v[3]);
        return result;
    }
};

// 2D point in Cartesian coordinates
struct vec2
{
    float x, y;
    
    vec2(float x = 0.0, float y = 0.0) : x(x), y(y) {}
    
    vec2 operator+(const vec2& v)
    {
        return vec2(x + v.x, y + v.y);
    }
    
    vec2 operator*(float s)
    {
        return vec2(x * s, y * s);
    }
    
};

// 3D point in Cartesian coordinates
struct vec3
{
    float x, y, z;
    
    vec3(float x = 0.0, float y = 0.0, float z = 0.0) : x(x), y(y), z(z) {}
    
    static vec3 random() { return vec3(((float)rand() / RAND_MAX) * 2 - 1, ((float)rand() / RAND_MAX) * 2 - 1, ((float)rand() / RAND_MAX) * 2 - 1); }
    
    vec3 operator+(const vec3& v) { return vec3(x + v.x, y + v.y, z + v.z); }
    
    vec3 operator-(const vec3& v) { return vec3(x - v.x, y - v.y, z - v.z); }
    
    vec3 operator*(float s) { return vec3(x * s, y * s, z * s); }
    
    vec3 operator/(float s) { return vec3(x / s, y / s, z / s); }
    
    float length() { return sqrt(x * x + y * y + z * z); }
    
    vec3 normalize() { return *this / length(); }
    
    void print() { printf("%f \t %f \t %f \n", x, y, z); }
};

vec3 cross(const vec3& a, const vec3& b)
{
    return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x );
}



class Geometry
{
protected:
    unsigned int vao;
    
public:
    Geometry()
    {
        glGenVertexArrays(1, &vao);
    }
    
    virtual void Draw() = 0;
};

class TexturedQuad : public Geometry
{
    unsigned int vbo[3];
    
public:
    TexturedQuad()
    {
        glBindVertexArray(vao);
        glGenBuffers(3, vbo);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        static float vertexCoords[] = {-1, 0, -1,        1, 0, -1,        -1, 0, 1,        1, 0, 1};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        static float vertexTexCoords[] = {0, 0,        10, 0,        0, 10,        10, 10};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexTexCoords), vertexTexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
        static float vertexNormals[] = {0, 1, 0,    0, 1, 0,    0, 1, 0,    0, 1, 0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexNormals), vertexNormals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
};

class InfiniteTexturedQuad : public Geometry
{
    unsigned int vbo[3];
    
public:
    InfiniteTexturedQuad()
    {
        glBindVertexArray(vao);
        glGenBuffers(3, vbo);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        static float vertexCoords[] = {0, 0, 0, 1,        -1, 0, -1, 0,        1, 0, -1, 0,        1, 0, 1, 0,        -1, 0, 1, 0,        -1, 0, -1, 0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        static float vertexTexCoords[] = {5, 5,        0, 0,        10, 0,        10, 10,        0, 10,        0, 0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexTexCoords), vertexTexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
        static float vertexNormals[] = {0, 1, 0,    0, 1, 0,    0, 1, 0,    0, 1, 0,    0, 1, 0,    0, 1, 0};
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexNormals), vertexNormals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
};

class   PolygonalMesh : public Geometry
{
    struct  Face
    {
        int       positionIndices[4];
        int       normalIndices[4];
        int       texcoordIndices[4];
        bool      isQuad;
    };
    
    std::vector<std::string*> rows;
    std::vector<vec3*> positions;
    std::vector<std::vector<Face*>> submeshFaces;
    std::vector<vec3*> normals;
    std::vector<vec2*> texcoords;
    
    int nTriangles;
    
public:
    PolygonalMesh(const char *filename);
    ~PolygonalMesh();
    
    void Draw();
};



PolygonalMesh::PolygonalMesh(const char *filename)
{
    std::fstream file(filename);
    if(!file.is_open())
    {
        return;
    }
    
    char buffer[256];
    while(!file.eof())
    {
        file.getline(buffer,256);
        rows.push_back(new std::string(buffer));
    }
    
    submeshFaces.push_back(std::vector<Face*>());
    std::vector<Face*>* faces = &submeshFaces.at(submeshFaces.size()-1);
    
    for(int i = 0; i < rows.size(); i++)
    {
        if(rows[i]->empty() || (*rows[i])[0] == '#')
        continue;
        else if((*rows[i])[0] == 'v' && (*rows[i])[1] == ' ')
        {
            float tmpx,tmpy,tmpz;
            sscanf(rows[i]->c_str(), "v %f %f %f" ,&tmpx,&tmpy,&tmpz);
            positions.push_back(new vec3(tmpx,tmpy,tmpz));
        }
        else if((*rows[i])[0] == 'v' && (*rows[i])[1] == 'n')
        {
            float tmpx,tmpy,tmpz;
            sscanf(rows[i]->c_str(), "vn %f %f %f" ,&tmpx,&tmpy,&tmpz);
            normals.push_back(new vec3(tmpx,tmpy,tmpz));
        }
        else if((*rows[i])[0] == 'v' && (*rows[i])[1] == 't')
        {
            float tmpx,tmpy;
            sscanf(rows[i]->c_str(), "vt %f %f" ,&tmpx,&tmpy);
            texcoords.push_back(new vec2(tmpx,tmpy));
        }
        else if((*rows[i])[0] == 'f')
        {
            if(count(rows[i]->begin(),rows[i]->end(), ' ') == 3)
            {
                Face* f = new Face();
                f->isQuad = false;
                sscanf(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d",
                       &f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
                       &f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
                       &f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2]);
                faces->push_back(f);
            }
            else
            {
                Face* f = new Face();
                f->isQuad = true;
                sscanf(rows[i]->c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d",
                       &f->positionIndices[0], &f->texcoordIndices[0], &f->normalIndices[0],
                       &f->positionIndices[1], &f->texcoordIndices[1], &f->normalIndices[1],
                       &f->positionIndices[2], &f->texcoordIndices[2], &f->normalIndices[2],
                       &f->positionIndices[3], &f->texcoordIndices[3], &f->normalIndices[3]);
                faces->push_back(f);
            }
        }
        else if((*rows[i])[0] == 'g')
        {
            if(faces->size() > 0)
            {
                submeshFaces.push_back(std::vector<Face*>());
                faces = &submeshFaces.at(submeshFaces.size()-1);
            }
        }
    }
    
    int numberOfTriangles = 0;
    for(int iSubmesh=0; iSubmesh<submeshFaces.size(); iSubmesh++)
    {
        std::vector<Face*>& faces = submeshFaces.at(iSubmesh);
        
        for(int i=0;i<faces.size();i++)
        {
            if(faces[i]->isQuad) numberOfTriangles += 2;
            else numberOfTriangles += 1;
        }
    }
    
    nTriangles = numberOfTriangles;
    
    float *vertexCoords = new float[numberOfTriangles * 9];
    float *vertexTexCoords = new float[numberOfTriangles * 6];
    float *vertexNormalCoords = new float[numberOfTriangles * 9];
    
    
    int triangleIndex = 0;
    for(int iSubmesh=0; iSubmesh<submeshFaces.size(); iSubmesh++)
    {
        std::vector<Face*>& faces = submeshFaces.at(iSubmesh);
        
        for(int i=0;i<faces.size();i++)
        {
            if(faces[i]->isQuad)
            {
                vertexTexCoords[triangleIndex * 6] =     texcoords[faces[i]->texcoordIndices[0]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 1] = 1-texcoords[faces[i]->texcoordIndices[0]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[1]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 3] = 1-texcoords[faces[i]->texcoordIndices[1]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[2]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 5] = 1-texcoords[faces[i]->texcoordIndices[2]-1]->y;
                
                
                vertexCoords[triangleIndex * 9] =     positions[faces[i]->positionIndices[0]-1]->x;
                vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[0]-1]->y;
                vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[0]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[1]-1]->x;
                vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[1]-1]->y;
                vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[1]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[2]-1]->x;
                vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[2]-1]->y;
                vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[2]-1]->z;
                
                
                vertexNormalCoords[triangleIndex * 9] =     normals[faces[i]->normalIndices[0]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[0]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[0]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[1]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[1]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[1]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[2]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[2]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[2]-1]->z;
                
                triangleIndex++;
                
                
                vertexTexCoords[triangleIndex * 6] =     texcoords[faces[i]->texcoordIndices[1]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 1] = 1-texcoords[faces[i]->texcoordIndices[1]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[2]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 3] = 1-texcoords[faces[i]->texcoordIndices[2]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[3]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 5] = 1-texcoords[faces[i]->texcoordIndices[3]-1]->y;
                
                
                vertexCoords[triangleIndex * 9] =     positions[faces[i]->positionIndices[1]-1]->x;
                vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[1]-1]->y;
                vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[1]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[2]-1]->x;
                vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[2]-1]->y;
                vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[2]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[3]-1]->x;
                vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[3]-1]->y;
                vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[3]-1]->z;
                
                
                vertexNormalCoords[triangleIndex * 9] =     normals[faces[i]->normalIndices[1]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[1]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[1]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[2]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[2]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[2]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[3]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[3]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[3]-1]->z;
                
                triangleIndex++;
            }
            else
            {
                vertexTexCoords[triangleIndex * 6] =     texcoords[faces[i]->texcoordIndices[0]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 1] = 1-texcoords[faces[i]->texcoordIndices[0]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 2] = texcoords[faces[i]->texcoordIndices[1]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 3] = 1-texcoords[faces[i]->texcoordIndices[1]-1]->y;
                
                vertexTexCoords[triangleIndex * 6 + 4] = texcoords[faces[i]->texcoordIndices[2]-1]->x;
                vertexTexCoords[triangleIndex * 6 + 5] = 1-texcoords[faces[i]->texcoordIndices[2]-1]->y;
                
                vertexCoords[triangleIndex * 9] =     positions[faces[i]->positionIndices[0]-1]->x;
                vertexCoords[triangleIndex * 9 + 1] = positions[faces[i]->positionIndices[0]-1]->y;
                vertexCoords[triangleIndex * 9 + 2] = positions[faces[i]->positionIndices[0]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 3] = positions[faces[i]->positionIndices[1]-1]->x;
                vertexCoords[triangleIndex * 9 + 4] = positions[faces[i]->positionIndices[1]-1]->y;
                vertexCoords[triangleIndex * 9 + 5] = positions[faces[i]->positionIndices[1]-1]->z;
                
                vertexCoords[triangleIndex * 9 + 6] = positions[faces[i]->positionIndices[2]-1]->x;
                vertexCoords[triangleIndex * 9 + 7] = positions[faces[i]->positionIndices[2]-1]->y;
                vertexCoords[triangleIndex * 9 + 8] = positions[faces[i]->positionIndices[2]-1]->z;
                
                
                vertexNormalCoords[triangleIndex * 9] =     normals[faces[i]->normalIndices[0]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 1] = normals[faces[i]->normalIndices[0]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 2] = normals[faces[i]->normalIndices[0]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 3] = normals[faces[i]->normalIndices[1]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 4] = normals[faces[i]->normalIndices[1]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 5] = normals[faces[i]->normalIndices[1]-1]->z;
                
                vertexNormalCoords[triangleIndex * 9 + 6] = normals[faces[i]->normalIndices[2]-1]->x;
                vertexNormalCoords[triangleIndex * 9 + 7] = normals[faces[i]->normalIndices[2]-1]->y;
                vertexNormalCoords[triangleIndex * 9 + 8] = normals[faces[i]->normalIndices[2]-1]->z;
                
                triangleIndex++;
            }
        }
    }
    
    glBindVertexArray(vao);
    
    unsigned int vbo[3];
    glGenBuffers(3, &vbo[0]);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, nTriangles * 9 * sizeof(float), vertexCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, nTriangles * 6 * sizeof(float), vertexTexCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ARRAY_BUFFER, nTriangles * 9 * sizeof(float), vertexNormalCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    delete vertexCoords;
    delete vertexTexCoords;
    delete vertexNormalCoords;
}


void PolygonalMesh::Draw()
{
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, nTriangles * 3);
    glDisable(GL_DEPTH_TEST);
}


PolygonalMesh::~PolygonalMesh()
{
    for(unsigned int i = 0; i < rows.size(); i++) delete rows[i];
    for(unsigned int i = 0; i < positions.size(); i++) delete positions[i];
    for(unsigned int i = 0; i < submeshFaces.size(); i++)
    for(unsigned int j = 0; j < submeshFaces.at(i).size(); j++)
    delete submeshFaces.at(i).at(j);
    for(unsigned int i = 0; i < normals.size(); i++) delete normals[i];
    for(unsigned int i = 0; i < texcoords.size(); i++) delete texcoords[i];
}

class Shader
{
protected:
    unsigned int shaderProgram;
    
public:
    Shader()
    {
        shaderProgram = 0;
    }
    
    ~Shader()
    {
        if(shaderProgram) glDeleteProgram(shaderProgram);
    }
    
    void Run()
    {
        if(shaderProgram) glUseProgram(shaderProgram);
    }
    
    virtual void UploadM(mat4& M) { }
    virtual void UploadInvM(mat4& InVM) { }
    virtual void UploadMVP(mat4& MVP) { }
    virtual void UploadVP(mat4& VP) { }
    virtual void UploadColor(vec4& color) { }
    virtual void UploadSamplerID() { }
    virtual void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) { }
    virtual void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) { }
    virtual void UploadEyePosition(vec3 wEye) { }
    virtual void UploadSamplerCubeID() { }
    virtual void UploadViewDirMatrix(mat4& viewDirMatrix) { }
    virtual void UploadEnvironmentIndicator(int indicator) {}
};

class MeshShader : public Shader
{
public:
    MeshShader()
    {
        const char *vertexSource = R"(
        #version 410
        precision highp float;
        in vec3 vertexPosition;
        in vec2 vertexTexCoord;
        in vec3 vertexNormal;
        uniform mat4 M, InvM, MVP;
        uniform vec3 worldEyePosition;
        uniform vec4 worldLightPosition;
        out vec2 texCoord;
        out vec3 worldNormal;
        out vec3 worldView;
        out vec3 worldLight;
        
        void main() {
            texCoord = vertexTexCoord;
            vec4 worldPosition = vec4(vertexPosition, 1) * M;
            worldLight  = worldLightPosition.xyz * worldPosition.w - worldPosition.xyz * worldLightPosition.w;
            worldView = worldEyePosition - worldPosition.xyz;
            worldNormal = (InvM * vec4(vertexNormal, 0.0)).xyz;
            gl_Position = vec4(vertexPosition, 1) * MVP;
        }
        )";
        
        const char *fragmentSource = R"(
        #version 410
        precision highp float;
        uniform sampler2D samplerUnit;
        uniform samplerCube environmentMap;
        uniform vec3 La, Le;
        uniform vec3 ka, kd, ks;
        uniform float shininess;
        uniform bool envIndicator;
        in vec2 texCoord;
        in vec3 worldNormal;
        in vec3 worldView;
        in vec3 worldLight;
        out vec4 fragmentColor;
        
        void main() {
            vec3 N = normalize(worldNormal);
            vec3 V = normalize(worldView);
            vec3 L = normalize(worldLight);
            vec3 H = normalize(V + L);
            vec3 texel = texture(samplerUnit, texCoord).xyz;
            
            vec3 color = La * ka + Le * kd * texel * max(0.0, dot(L, N)) + Le * ks * pow(max(0.0, dot(H, N)), shininess);
            
            if (envIndicator) {
                vec3 R = N * dot(N, V) * 2.0 - V;
                vec3 texel2 = texture(environmentMap, R).xyz;
                color =
                La * ka +
                (Le * kd * texel * max(0.0, dot(L, N))) * 0.5 + texel2 * 0.5 +
                Le * ks * pow(max(0.0, dot(H, N)), shininess);
            }
            
            fragmentColor = vec4(color, 1);
        }
        )";
        
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
        
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "Vertex shader error");
        
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
        
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "Fragment shader error");
        
        shaderProgram = glCreateProgram();
        if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
        
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        
        glBindAttribLocation(shaderProgram, 0, "vertexPosition");
        glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
        glBindAttribLocation(shaderProgram, 2, "vertexNormal");
        
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
        
        glLinkProgram(shaderProgram);
        checkLinking(shaderProgram);
    }
    
    void UploadSamplerID()
    {
        int samplerUnit = 0;
        int location = glGetUniformLocation(shaderProgram, "samplerUnit");
        glUniform1i(location, samplerUnit);
        glActiveTexture(GL_TEXTURE0 + samplerUnit);
    }
    
    
    void UploadSamplerCubeID() {
        int samplerCube = 1;
        int location = glGetUniformLocation(shaderProgram, "environmentMap");
        glUniform1i(location, samplerCube);
        glActiveTexture(GL_TEXTURE0 + samplerCube);

    }
    
    void UploadM(mat4& M)
    {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadInvM(mat4& InvM)
    {
        int location = glGetUniformLocation(shaderProgram, "InvM");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, InvM);
        else printf("uniform InvM cannot be set\n");
    }
    
    void UploadMVP(mat4& MVP)
    {
        int location = glGetUniformLocation(shaderProgram, "MVP");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVP);
        else printf("uniform MVP cannot be set\n");
    }
    
    void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) {
        
        int location1 = glGetUniformLocation(shaderProgram, "ka");
        if (location1 >= 0) glUniform3fv(location1, 1, &ka.x);
        else printf("uniform ka cannot be set\n");
        
        int location2 = glGetUniformLocation(shaderProgram, "kd");
        if (location2 >= 0) glUniform3fv(location2, 1, &kd.x);
        else printf("uniform kd cannot be set\n");
        
        int location3 = glGetUniformLocation(shaderProgram, "ks");
        if (location3 >= 0) glUniform3fv(location3, 1, &ks.x);
        else printf("uniform ks cannot be set\n");
        
        int location4 = glGetUniformLocation(shaderProgram, "shininess");
        if (location4 >= 0) glUniform1f(location4, shininess);
        else printf("uniform shininess cannot be set\n");
    }
    
    void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {
        
        int location1 = glGetUniformLocation(shaderProgram, "La");
        if (location1 >= 0) glUniform3fv(location1, 1, &La.x);
        else printf("uniform La cannot be set\n");
        
        int location2 = glGetUniformLocation(shaderProgram, "Le");
        if (location2 >= 0) glUniform3fv(location2, 1, &Le.x);
        else printf("uniform Le cannot be set\n");
        
        int location3 = glGetUniformLocation(shaderProgram, "worldLightPosition");
        if (location3 >= 0) glUniform4fv(location3, 1, &worldLightPosition.v[0]);
        else printf("uniform world light position cannot be set\n");
    }
    
    void UploadEyePosition(vec3 wEye) {
        int location = glGetUniformLocation(shaderProgram, "worldEyePosition");
        if (location >= 0) glUniform3fv(location, 1, &wEye.x);
        else printf("uniform eye position cannot be set\n");
    }
    
    void UploadEnvironmentIndicator(int indicator) {
        int location = glGetUniformLocation(shaderProgram, "envIndicator");
        if (location >= 0) glUniform1i (location, indicator);
        else printf("environment indicator cannot be set\n");
    }

};


class InfiniteQuadShader : public Shader
{
public:
    InfiniteQuadShader()
    {
        const char *vertexSource = R"(
        #version 410
        in vec4 vertexPosition;
        in vec2 vertexTexCoord;
        in vec3 vertexNormal;
        uniform mat4 M, InvM, MVP;
        
        out vec2 texCoord;
        out vec4 worldPosition;
        out vec3 worldNormal;
        
        void main() {
            texCoord = vertexTexCoord;
            worldPosition = vertexPosition * M;
            worldNormal = (InvM * vec4(vertexNormal, 0.0)).xyz;
            gl_Position = vertexPosition * MVP;
        }
        )";
        
        const char *fragmentSource = R"(
        #version 410
        precision highp float;
        uniform sampler2D samplerUnit;
        uniform vec3 La, Le;
        uniform vec3 ka, kd, ks;
        uniform float shininess;
        uniform vec3 worldEyePosition;
        uniform vec4 worldLightPosition;
        in vec2 texCoord;
        in vec4 worldPosition;
        in vec3 worldNormal;
        out vec4 fragmentColor;
        void main() {
            vec3 N = normalize(worldNormal);
            vec3 V = normalize(worldEyePosition * worldPosition.w - worldPosition.xyz);
            vec3 L = normalize(worldLightPosition.xyz * worldPosition.w - worldPosition.xyz * worldLightPosition.w);
            vec3 H = normalize(V + L);
            vec2 position = worldPosition.xz / worldPosition.w;
            vec2 tex = position.xy - floor(position.xy);
            vec3 texel = texture(samplerUnit, tex).xyz;
            vec3 color = La * ka + Le * kd * texel * max(0.0, dot(L, N)) + Le * ks * pow(max(0.0, dot(H, N)), shininess);
            fragmentColor = vec4(color, 1);
        }
        )";
        
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
        
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "Vertex shader error");
        
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
        
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "Fragment shader error");
        
        shaderProgram = glCreateProgram();
        if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
        
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        
        glBindAttribLocation(shaderProgram, 0, "vertexPosition");
        glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
        glBindAttribLocation(shaderProgram, 2, "vertexNormal");
        
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
        
        glLinkProgram(shaderProgram);
        checkLinking(shaderProgram);
    }
    
    void UploadSamplerID()
    {
        int samplerUnit = 0;
        int location = glGetUniformLocation(shaderProgram, "samplerUnit");
        glUniform1i(location, samplerUnit);
        glActiveTexture(GL_TEXTURE0 + samplerUnit);
    }
    
    void UploadM(mat4& M)
    {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadInvM(mat4& InvM)
    {
        int location = glGetUniformLocation(shaderProgram, "InvM");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, InvM);
        else printf("uniform InvM cannot be set\n");
    }
    
    void UploadMVP(mat4& MVP)
    {
        int location = glGetUniformLocation(shaderProgram, "MVP");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, MVP);
        else printf("uniform MVP cannot be set\n");
    }
    
    void UploadMaterialAttributes(vec3 ka, vec3 kd, vec3 ks, float shininess) {
        
        int location1 = glGetUniformLocation(shaderProgram, "ka");
        if (location1 >= 0) glUniform3fv(location1, 1, &ka.x);
        else printf("uniform ka cannot be set\n");
        
        int location2 = glGetUniformLocation(shaderProgram, "kd");
        if (location2 >= 0) glUniform3fv(location2, 1, &kd.x);
        else printf("uniform kd cannot be set\n");
        
        int location3 = glGetUniformLocation(shaderProgram, "ks");
        if (location3 >= 0) glUniform3fv(location3, 1, &ks.x);
        else printf("uniform ks cannot be set\n");
        
        int location4 = glGetUniformLocation(shaderProgram, "shininess");
        if (location4 >= 0) glUniform1f(location4, shininess);
        else printf("uniform shininess cannot be set\n");
    }
    
    void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition) {
        
        int location1 = glGetUniformLocation(shaderProgram, "La");
        if (location1 >= 0) glUniform3fv(location1, 1, &La.x);
        else printf("uniform La cannot be set\n");
        
        int location2 = glGetUniformLocation(shaderProgram, "Le");
        if (location2 >= 0) glUniform3fv(location2, 1, &Le.x);
        else printf("uniform Le cannot be set\n");
        
        int location3 = glGetUniformLocation(shaderProgram, "worldLightPosition");
        if (location3 >= 0) glUniform4fv(location3, 1, &worldLightPosition.v[0]);
        else printf("uniform world light position cannot be set\n");
    }
    
    void UploadEyePosition(vec3 wEye) {
        int location = glGetUniformLocation(shaderProgram, "worldEyePosition");
        if (location >= 0) glUniform3fv(location, 1, &wEye.x);
        else printf("uniform eye position cannot be set\n");
    }
};

class ShadowShader : public Shader
{
public:
    ShadowShader()
    {
        const char *vertexSource = R"(
        #version 410
        precision highp float;
        
        in vec3 vertexPosition;
        in vec2 vertexTexCoord;
        in vec3 vertexNormal;
        uniform mat4 M, VP;
        uniform vec4 worldLightPosition;
        
        void main() {
            vec4 p = vec4(vertexPosition, 1) * M;
            vec3 s;
            s.y = -0.999;
            s.x = (p.x - worldLightPosition.x) / (p.y - worldLightPosition.y) * (s.y - worldLightPosition.y) + worldLightPosition.x;
            s.z = (p.z - worldLightPosition.z) / (p.y - worldLightPosition.y) * (s.y - worldLightPosition.y) + worldLightPosition.z;
            gl_Position = vec4(s, 1) * VP;
        }
        )";
        
        const char *fragmentSource = R"(
        #version 410
        precision highp float;
        
        out vec4 fragmentColor;
        
        void main()
        {
            fragmentColor = vec4(0.0, 0.1, 0.0, 1);
        }
        )";
        
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
        
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "Vertex shader error");
        
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
        
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "Fragment shader error");
        
        shaderProgram = glCreateProgram();
        if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
        
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        
        glBindAttribLocation(shaderProgram, 0, "vertexPosition");
        glBindAttribLocation(shaderProgram, 1, "vertexTexCoord");
        glBindAttribLocation(shaderProgram, 2, "vertexNormal");
        
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
        
        glLinkProgram(shaderProgram);
        checkLinking(shaderProgram);
    }
    
    void UploadM(mat4& M)
    {
        int location = glGetUniformLocation(shaderProgram, "M");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, M);
        else printf("uniform M cannot be set\n");
    }
    
    void UploadVP(mat4& VP)
    {
        int location = glGetUniformLocation(shaderProgram, "VP");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, VP);
        else printf("uniform VP cannot be set\n");
    }
    
    void UploadLightAttributes(vec3 La, vec3 Le, vec4 worldLightPosition)
    {
        int location = glGetUniformLocation(shaderProgram, "worldLightPosition");
        if (location >= 0) glUniform4fv(location, 1, &worldLightPosition.v[0]);
        else printf("uniform world light position cannot be set\n");
    }
};

class EnvironmentShader : public Shader
{
public:
    EnvironmentShader()
    {
        const char *vertexSource = R"(
#version 410
        precision highp float;
        
        in vec4 vertexPosition;
        uniform mat4 viewDirMatrix;
        
        out vec4 worldPosition;
        out vec3 viewDir;
        
        void main()
        {
            worldPosition = vertexPosition;
            viewDir = (vertexPosition * viewDirMatrix).xyz;
            gl_Position = vertexPosition;
            gl_Position.z = 0.999999;
        }
        )";
        
        const char *fragmentSource = R"(
        #version 410
        precision highp float;
        
        uniform samplerCube environmentMap;
        in vec4 worldPosition;
        in vec3 viewDir;
        out vec4 fragmentColor;
        
        void main()
        {
            vec3 texel = texture(environmentMap, viewDir).xyz;
            fragmentColor = vec4(texel, 1);
        }
        )";
        
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        if (!vertexShader) { printf("Error in vertex shader creation\n"); exit(1); }
        
        glShaderSource(vertexShader, 1, &vertexSource, NULL);
        glCompileShader(vertexShader);
        checkShader(vertexShader, "Vertex shader error");
        
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        if (!fragmentShader) { printf("Error in fragment shader creation\n"); exit(1); }
        
        glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
        glCompileShader(fragmentShader);
        checkShader(fragmentShader, "Fragment shader error");
        
        shaderProgram = glCreateProgram();
        if (!shaderProgram) { printf("Error in shader program creation\n"); exit(1); }
        
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        
        glBindAttribLocation(shaderProgram, 0, "vertexPosition");
        
        glBindFragDataLocation(shaderProgram, 0, "fragmentColor");
        
        glLinkProgram(shaderProgram);
        checkLinking(shaderProgram);
    }
    
    
    void UploadSamplerCubeID() {
        int samplerCube = 1;
        int location = glGetUniformLocation(shaderProgram, "environmentMap");
        glUniform1i(location, samplerCube);
        glActiveTexture(GL_TEXTURE0 + samplerCube);
        
    }
    
    void UploadViewDirMatrix(mat4& viewDirMatrix) {
        int location = glGetUniformLocation(shaderProgram, "viewDirMatrix");
        if (location >= 0) glUniformMatrix4fv(location, 1, GL_TRUE, viewDirMatrix);
        else printf("View direction matrix cannot be set\n");
    }
    
};

extern "C" unsigned char* stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp);

class Texture
{
    unsigned int textureId;
    
public:
    Texture(const std::string& inputFileName)
    {
        unsigned char* data;
        int width; int height; int nComponents = 4;
        
        data = stbi_load(inputFileName.c_str(), &width, &height, &nComponents, 0);
        
        if(data == NULL)
        {
            return;
        }
        
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);
        
        if(nComponents == 3) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        if(nComponents == 4) glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        delete data;
    }
    
    void Bind()
    {
        glBindTexture(GL_TEXTURE_2D, textureId);
    }
};

class TextureCube {
    unsigned int textureId;
public:
    TextureCube(
                const std::string& inputFileName0, const std::string& inputFileName1, const std::string& inputFileName2,
                const std::string& inputFileName3, const std::string& inputFileName4, const std::string& inputFileName5) {
        unsigned char* data[6]; int width[6]; int height[6]; int nComponents[6]; std::string filename[6];
        filename[0] = inputFileName0; filename[1] = inputFileName1; filename[2] = inputFileName2;
        filename[3] = inputFileName3; filename[4] = inputFileName4; filename[5] = inputFileName5;
        for(int i = 0; i < 6; i++) {
            data[i] = stbi_load(filename[i].c_str(), &width[i], &height[i], &nComponents[i], 0);
            if(data[i] == NULL) printf("Textures not loaded");
            if(data == NULL) {
                return;
            }
        }
        glGenTextures(1, &textureId); glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
        for(int i = 0; i < 6; i++) {
            if(nComponents[i] == 4) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width[i], height[i], 0, GL_RGBA, GL_UNSIGNED_BYTE, data[i]);
            if(nComponents[i] == 3) glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, GL_RGB, GL_UNSIGNED_BYTE, data[i]);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        for(int i = 0; i < 6; i++)  delete data[i];
    }
    
    void Bind() { glBindTexture(GL_TEXTURE_CUBE_MAP, textureId); }
};

class Material
{
    Shader* shader;
    Texture* texture;
    vec3 ka, kd, ks;
    float shininess;
    TextureCube* environmentMap;
    
public:
    Material(Shader* s, vec3 ka, vec3 kd, vec3 ks, float shininess, Texture* texture = 0,
             TextureCube* e = 0) :
        shader(s), ka(ka), kd(kd), ks(ks), shininess(shininess), texture(texture), environmentMap(e){}
    
    Shader* GetShader() { return shader; }
    
    void UploadAttributes()
    {
        if(texture){
            shader->UploadSamplerID();
            texture->Bind();
            shader->UploadMaterialAttributes(ka, kd, ks, shininess);
        }
        if(environmentMap){
            shader->UploadSamplerCubeID();
            environmentMap->Bind();
            shader->UploadEnvironmentIndicator(1);
        }
        else {
            shader->UploadEnvironmentIndicator(0);
        }
    }
};

class Light
{
    vec3 La, Le;
    vec4 worldLightPosition;
    
public:
    Light(vec3 La, vec3 Le, vec4 worldLightPosition):
    La(La), Le(Le), worldLightPosition(worldLightPosition) {}
    
    void UploadAttributes(Shader* shader)
    {
        shader->UploadLightAttributes(La, Le, worldLightPosition);
    }
    
    void SetPointLightSource(vec3& pos) {
        worldLightPosition = vec4(pos.x, pos.y, pos.z, 1);
    }
    
    void SetDirectionalLightSource(vec3& dir) {
        worldLightPosition = vec4(dir.x, dir.y, dir.z, 0);
    }
};

class Mesh
{
    Geometry* geometry;
    Material* material;
    
public:
    Mesh(Geometry* g, Material* m)
    {
        geometry = g;
        material = m;
    }
    
    Shader* GetShader() { return material->GetShader(); }
    
    void Draw()
    {
        material->UploadAttributes();
        geometry->Draw();
    }
};

class Camera {
    vec3  wEye, wLookat, wVup;
    float fov, asp, fp, bp;
    vec3 velocity;
    /**float force = 1;
    float speed = 0.6;
    float invMass = 0.3;
    float acceleration;**/
    
public:
    Camera()
    {
        wEye = vec3(0.0, 0.0, 2.0);
        wLookat = vec3(0.0, 0.0, 0.0);
        wVup = vec3(0.0, 1.0, 0.0);
        fov = M_PI / 4.0; asp = 1.0; fp = 0.01; bp = 10.0;
    }
    
    void SetAspectRatio(float a) { asp = a; }
    
    void Control() {
        velocity = (wLookat - wEye).normalize() * 5;
    }
    
    void MoveHelicam(vec3 pos, float orient, float dt) {
        //force = force + 4*dt;
        //acceleration = force*invMass;
        //speed = speed - acceleration * dt;
        
        float radians = orient * (M_PI/180);
        wEye = pos + vec3(2*cosf(radians), 2, 2*sinf(radians));
        wLookat = pos + vec3(0, 1.5, 0);
    }
    
    void Move(float dt) {
        Control();
        if (keyboardState['i']) {
            wEye = wEye + (velocity * dt);
        }
        if (keyboardState['k']) {
            wEye = wEye - (velocity * dt);
        }
        if (keyboardState['j']) {
            vec3 ahead = (wLookat - wEye).normalize();
            vec3 right = cross(wVup, ahead).normalize();
            vec3 ahead_prime = ahead*cosf(0.5 * dt) + right*sinf(0.5 * dt);
            wLookat = wEye + ahead_prime * (wLookat - wEye).length();
        }
        if (keyboardState['l']) {
            vec3 ahead = (wLookat - wEye).normalize();
            vec3 right = cross(wVup, ahead).normalize();
            vec3 ahead_prime = ahead*cosf(-0.5 * dt) + right*sinf(-0.5 * dt);
            wLookat = wEye + ahead_prime * (wLookat - wEye).length();
        }
    }
    
    mat4 GetViewMatrix()
    {
        vec3 w = (wEye - wLookat).normalize();
        vec3 u = cross(wVup, w).normalize();
        vec3 v = cross(w, u);
        
        return
        mat4(
             1.0f,    0.0f,    0.0f,    0.0f,
             0.0f,    1.0f,    0.0f,    0.0f,
             0.0f,    0.0f,    1.0f,    0.0f,
             -wEye.x, -wEye.y, -wEye.z, 1.0f ) *
        mat4(
             u.x,  v.x,  w.x,  0.0f,
             u.y,  v.y,  w.y,  0.0f,
             u.z,  v.z,  w.z,  0.0f,
             0.0f, 0.0f, 0.0f, 1.0f );
    }
    
    mat4 GetProjectionMatrix()
    {
        float sy = 1/tan(fov/2);
        return mat4(
                    sy/asp, 0.0f,  0.0f,               0.0f,
                    0.0f,   sy,    0.0f,               0.0f,
                    0.0f,   0.0f, -(fp+bp)/(bp - fp), -1.0f,
                    0.0f,   0.0f, -2*fp*bp/(bp - fp),  0.0f);
    }
    
    mat4 GetInverseViewMatrix() {
        vec3 w = (wEye - wLookat).normalize();
        vec3 u = cross(wVup, w).normalize();
        vec3 v = cross(w, u);
        return mat4(
                    u.x,  u.y,  u.z,  0.0f,
                    v.x,  v.y,  v.z,  0.0f,
                    w.x,  w.y,  w.z,  0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f );
    }
    mat4 GetInverseProjectionMatrix() {
        float sy = 1/tan(fov/2);
        return mat4(
                    asp/sy,        0.0f,        0.0f,        0.0f,
                    0.0f,        1.0/sy,        0.0f,        0.0f,
                    0.0f,        0.0f,        0.0f,        (fp - bp) / 2.0 / fp / bp,
                    0.0f,        0.0f,        -1.0f,        (fp + bp) / 2.0 / fp / bp);
    }
    
    void UploadAttributes(Shader *shader) {
        shader->UploadEyePosition(wEye);
    }
    
    vec3 GetEyePosition() {
        return wEye;
    }
};

Camera camera;
vec3 wEye = camera.GetEyePosition();
Light light = Light(vec3(1, 1, 1), vec3(1, 1, 1), vec4(0.1, 0.1, 0.1, 0.0));//vec4(wEye.x, wEye.y, wEye.z, 0));

class Environment : public Geometry
{
    unsigned int vbo;
    EnvironmentShader *shader;
    TextureCube *environmentMap;
    
public:
    Environment(EnvironmentShader *s, TextureCube *e)
    {
        shader = s;
        environmentMap = e;
        
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        static float vertexCoords[] =     {
            -1,    -1,     0,     1,
            1,     -1,     0,     1,
            -1,      1,     0,     1,
            1,      1,     0,     1    };
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    
    void Draw()
    {
        shader->Run();
        environmentMap->Bind();
        mat4 viewDirMatrix = camera.GetInverseProjectionMatrix() * camera.GetInverseViewMatrix();
        shader->UploadViewDirMatrix(viewDirMatrix);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
    }
};

class Object{
    Shader* shader;
    Mesh *mesh;
    vec3 position;
    vec3 scaling;
    float orientation;
    
public:
    Object(Mesh *m, vec3 position = vec3(0.0, 0.0, 0.0), vec3 scaling = vec3(1.0, 1.0, 1.0), float orientation = 0.0) : position(position), scaling(scaling), orientation(orientation)
    {
        shader = m->GetShader();
        mesh = m;
    }
    
    virtual void UploadAttributes(Shader* s) {}
    virtual vec3& GetPosition() { return position; }
    virtual float GetOrientation() { return orientation; }
    virtual void Move(float dt) {}
    
    void Draw()
    {
        shader->Run();
        UploadAttributes(shader);
        light.UploadAttributes(shader);
        camera.UploadAttributes(shader);
        mesh->Draw();
    }
    
    void DrawShadow(Shader* shadowShader)
    {
        shadowShader->Run();
        
        UploadAttributes(shadowShader);
        
        vec3 source = vec3(0, 100, 0);
        light.SetPointLightSource(source);
        light.UploadAttributes(shadowShader);
        
        camera.UploadAttributes(shadowShader);
        
        mesh->Draw();
        
    }
    
};

class AvatarObject: public Object
{
    Shader* shader;
    Mesh *mesh;
    vec3 position;
    vec3 scaling;
    float orientation;
    
public:
    AvatarObject(Mesh *m, vec3 position = vec3(0.0, 0.0, 0.0), vec3 scaling = vec3(1.0, 1.0, 1.0), float orientation = 0.0) :
    Object(m, position, scaling, orientation), position(position), scaling(scaling), orientation(orientation)
    {
        shader = m->GetShader();
        mesh = m;
    }
    
    void UploadAttributes(Shader* s)
    {
        mat4 T = mat4(
                      1.0,            0.0,            0.0,            0.0,
                      0.0,            1.0,            0.0,            0.0,
                      0.0,            0.0,            1.0,            0.0,
                      position.x,        position.y,        position.z,        1.0);
        
        mat4 InvT = mat4(
                         1.0,            0.0,            0.0,            0.0,
                         0.0,            1.0,            0.0,            0.0,
                         0.0,            0.0,            1.0,            0.0,
                         -position.x,    -position.y,    -position.z,    1.0);
        
        mat4 S = mat4(
                      scaling.x,        0.0,            0.0,            0.0,
                      0.0,            scaling.y,        0.0,            0.0,
                      0.0,            0.0,            scaling.z,        0.0,
                      0.0,            0.0,            0.0,            1.0);
        
        mat4 InvS = mat4(
                         1.0/scaling.x,    0.0,            0.0,            0.0,
                         0.0,            1.0/scaling.y,    0.0,            0.0,
                         0.0,            0.0,            1.0/scaling.z,    0.0,
                         0.0,            0.0,            0.0,            1.0);
        
        float alpha = orientation / 180.0 * M_PI;
        
        mat4 R = mat4(
                      cos(alpha),        0.0,            sin(alpha),        0.0,
                      0.0,            1.0,            0.0,            0.0,
                      -sin(alpha),    0.0,            cos(alpha),        0.0,
                      0.0,            0.0,            0.0,            1.0);
        
        mat4 InvR = mat4(
                         cos(alpha),        0.0,            -sin(alpha),    0.0,
                         0.0,            1.0,            0.0,            0.0,
                         sin(alpha),        0.0,            cos(alpha),        0.0,
                         0.0,            0.0,            0.0,            1.0);
        
        mat4 M = S * R * T;
        mat4 InvM = InvT * InvR * InvS;
        
        mat4 MVP = M * camera.GetViewMatrix() * camera.GetProjectionMatrix();
        mat4 VP = camera.GetViewMatrix() * camera.GetProjectionMatrix();
        
        s->UploadM(M);
        s->UploadInvM(InvM);
        s->UploadMVP(MVP);
        s->UploadVP(VP);
    }
    
    vec3& GetPosition() { return position; }
    
    float GetOrientation() { return orientation; }
    
    void Move(float dt) {
        float radians = orientation * (M_PI/180);
        if (keyboardState['w']) {
            position.x = position.x - dt * cosf(radians);
            position.z = position.z - dt * sinf(radians);
        }
        if (keyboardState['s']) {
            position.x = position.x + dt * cosf(radians);
            position.z = position.z + dt * sinf(radians);
        }
        if (keyboardState['a']) {
            orientation = orientation - dt*50;
        }
        if (keyboardState['d']) {
            orientation = orientation + dt*50;
        }
    }

    
};

class BackgroundObject: public Object
{
    Shader* shader;
    Mesh *mesh;
    vec3 position;
    vec3 scaling;
    float orientation;
    
public:
    BackgroundObject(Mesh *m, vec3 position = vec3(0.0, 0.0, 0.0), vec3 scaling = vec3(1.0, 1.0, 1.0), float orientation = 0.0) :
    Object(m, position, scaling, orientation), position(position), scaling(scaling), orientation(orientation)
    {
        shader = m->GetShader();
        mesh = m;
    }
    
    void UploadAttributes(Shader* s)
    {
        mat4 T = mat4(
                      1.0,            0.0,            0.0,            0.0,
                      0.0,            1.0,            0.0,            0.0,
                      0.0,            0.0,            1.0,            0.0,
                      position.x,        position.y,        position.z,        1.0);
        
        mat4 InvT = mat4(
                         1.0,            0.0,            0.0,            0.0,
                         0.0,            1.0,            0.0,            0.0,
                         0.0,            0.0,            1.0,            0.0,
                         -position.x,    -position.y,    -position.z,    1.0);
        
        mat4 S = mat4(
                      scaling.x,        0.0,            0.0,            0.0,
                      0.0,            scaling.y,        0.0,            0.0,
                      0.0,            0.0,            scaling.z,        0.0,
                      0.0,            0.0,            0.0,            1.0);
        
        mat4 InvS = mat4(
                         1.0/scaling.x,    0.0,            0.0,            0.0,
                         0.0,            1.0/scaling.y,    0.0,            0.0,
                         0.0,            0.0,            1.0/scaling.z,    0.0,
                         0.0,            0.0,            0.0,            1.0);
        
        float alpha = orientation / 180.0 * M_PI;
        
        mat4 R = mat4(
                      cos(alpha),        0.0,            sin(alpha),        0.0,
                      0.0,            1.0,            0.0,            0.0,
                      -sin(alpha),    0.0,            cos(alpha),        0.0,
                      0.0,            0.0,            0.0,            1.0);
        
        mat4 InvR = mat4(
                         cos(alpha),        0.0,            -sin(alpha),    0.0,
                         0.0,            1.0,            0.0,            0.0,
                         sin(alpha),        0.0,            cos(alpha),        0.0,
                         0.0,            0.0,            0.0,            1.0);
        
        mat4 M = S * R * T;
        mat4 InvM = InvT * InvR * InvS;
        
        mat4 MVP = M * camera.GetViewMatrix() * camera.GetProjectionMatrix();
        mat4 VP = camera.GetViewMatrix() * camera.GetProjectionMatrix();
        
        s->UploadM(M);
        s->UploadInvM(InvM);
        s->UploadMVP(MVP);
        s->UploadVP(VP);
    }
    
    vec3& GetPosition() { return position; }
};

class Scene
{
    MeshShader *meshShader;
    InfiniteQuadShader *infiniteShader;
    ShadowShader *shadowShader;
    TextureCube *environmentMap;
    EnvironmentShader *envShader;
    
    std::vector<Texture*> textures;
    std::vector<Material*> materials;
    std::vector<Geometry*> geometries;
    std::vector<Mesh*> meshes;
    std::vector<Object*> objects;
    
    Environment *environment;
    
public:
    Scene()
    {
        meshShader = 0;
        infiniteShader = 0;
        shadowShader = 0;
        environmentMap = 0;
        envShader = 0;
    }
    
    void Initialize()
    {
        meshShader = new MeshShader();
        infiniteShader = new InfiniteQuadShader();
        shadowShader = new ShadowShader();
        
        environmentMap = new TextureCube("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/posx512.jpg",
                        "/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/negx512.jpg",
                        "/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/posy512.jpg",
                        "/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/negy512.jpg",
                        "/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/posz512.jpg",
                        "/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/negz512.jpg");
        
        envShader = new EnvironmentShader();
        
        vec3 ka = vec3(0.1, 0.1, 0.1);
        vec3 kd = vec3(0.6, 0.6, 0.6);
        vec3 ks = vec3(0.3, 0.3, 0.3);
        float shininess = 50;

        
        textures.push_back(new Texture("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/tigger.png"));
        materials.push_back(new Material(meshShader, ka, kd, ks, shininess, textures[0], environmentMap));
        geometries.push_back(new PolygonalMesh("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/tigger.obj"));
        meshes.push_back(new Mesh(geometries[0], materials[0]));
        Object* object = new AvatarObject(meshes[0], vec3(0.0, -1.0, 0.0), vec3(0.05, 0.05, 0.05), -60.0);
        objects.push_back(object);
        
        textures.push_back(new Texture("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/tree.png"));
        materials.push_back(new Material(meshShader, ka, kd, ks, shininess, textures[1]));
        geometries.push_back(new PolygonalMesh("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/tree.obj"));
        meshes.push_back(new Mesh(geometries[1], materials[1]));
        Object* object2 = new BackgroundObject(meshes[1], vec3(-0.5, -0.5, -0.1), vec3(0.015, 0.015, 0.015), -60.0);
        objects.push_back(object2);
        
        environment = new Environment(envShader, environmentMap);
        

        textures.push_back(new Texture("/Users/Tongyu/Documents/AIT_Budapest/Graphics/Meshes/Meshes/tree.png"));
        materials.push_back(new Material(infiniteShader, ka, kd, ks, shininess, textures[2]));
        geometries.push_back(new InfiniteTexturedQuad());
        meshes.push_back(new Mesh(geometries[2], materials[2]));
        Object* object3 = new BackgroundObject(meshes[2], vec3(0, -1, 0));
        objects.push_back(object3);
    }
    
    ~Scene()
    {
        for(int i = 0; i < textures.size(); i++) delete textures[i];
        for(int i = 0; i < materials.size(); i++) delete materials[i];
        for(int i = 0; i < geometries.size(); i++) delete geometries[i];
        for(int i = 0; i < meshes.size(); i++) delete meshes[i];
        for(int i = 0; i < objects.size(); i++) delete objects[i];
        
        if(meshShader) delete meshShader;
        if(infiniteShader) delete infiniteShader;
        if(shadowShader) delete shadowShader;
        if(environmentMap) delete environmentMap;
        if(envShader) delete envShader;
    }
    
    void Draw()
    {
        // last object is the ground

        for(int i = 0; i < objects.size() - 1; i++){
            objects[i]->DrawShadow(shadowShader);
        }
        for(int i = 0; i < objects.size(); i++){
            objects[i]->Draw();
        }
        environment->Draw();
        
    }
    
    void Move(float dt) {
        for(int i = 0; i < objects.size(); i++){
            objects[i]->Move(dt);
        }
    }
    
    Object* GetAvatar() {
        return objects[0];
    }
};

Scene scene;

void onInitialization()
{
    glViewport(0, 0, windowWidth, windowHeight);
    
    scene.Initialize();
}

void onExit()
{
    printf("exit");
}

void onDisplay()
{
    
    glClearColor(0.3, 0.48, 0.52, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    scene.Draw();
    
    glutSwapBuffers();
    
}

void onKeyboard(unsigned char key, int x, int y)
{
    keyboardState[key] = true;
}

void onKeyboardUp(unsigned char key, int x, int y)
{
    keyboardState[key] = false;
}

void onReshape(int winWidth, int winHeight)
{
    camera.SetAspectRatio((float)winWidth / winHeight);
    glViewport(0, 0, winWidth, winHeight);
}

void onIdle( ) {
    double t = glutGet(GLUT_ELAPSED_TIME) * 0.001;
    static double lastTime = 0.0;
    double dt = t - lastTime;
    lastTime = t;
    
    camera.MoveHelicam(scene.GetAvatar()->GetPosition(), scene.GetAvatar()->GetOrientation(), dt);
    //camera.Move(dt);
    scene.Move(dt);
    
    glutPostRedisplay();
}

int main(int argc, char * argv[])
{
    glutInit(&argc, argv);
#if !defined(__APPLE__)
    glutInitContextVersion(majorVersion, minorVersion);
#endif
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(50, 50);
#if defined(__APPLE__)
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutCreateWindow("3D Mesh Rendering");
    
#if !defined(__APPLE__)
    glewExperimental = true;
    glewInit();
#endif
    printf("GL Vendor    : %s\n", glGetString(GL_VENDOR));
    printf("GL Renderer  : %s\n", glGetString(GL_RENDERER));
    printf("GL Version (string)  : %s\n", glGetString(GL_VERSION));
    glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
    printf("GL Version (integer) : %d.%d\n", majorVersion, minorVersion);
    printf("GLSL Version : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    
    onInitialization();
    
    glutDisplayFunc(onDisplay);
    glutIdleFunc(onIdle);
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    glutReshapeFunc(onReshape);
    
    glutMainLoop();
    onExit();
    return 1;
}
