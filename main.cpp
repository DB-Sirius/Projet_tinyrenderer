#include <iostream>
#include "tgaimage.h"
#include "model.h"

const TGAColor white    = TGAColor(254, 255, 255, 255);
const TGAColor red      = TGAColor(255, 0,   0,   255);
const TGAColor blue     = TGAColor(0, 0,   255,   255);
const TGAColor green    = TGAColor(0, 255, 0,     255);
const TGAColor magenta  = TGAColor(255, 0,   255, 255); 
Model *model = NULL;
const int width = 800;
const int height = 800;


/**
 * Dessin d'une ligne avec l'algorithme de Bresenham
 
 */
void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) { 
    bool steep = false; 
    if (std::abs(x0-x1)<std::abs(y0-y1)) { 
        std::swap(x0, y0); 
        std::swap(x1, y1); 
        steep = true; 
    } 
    if (x0>x1) { 
        std::swap(x0, x1); 
        std::swap(y0, y1); 
    } 
    int dx = x1-x0; 
    int dy = y1-y0; 
    int derror2 = std::abs(dy)*2; 
    int error2 = 0; 
    int y = y0; 
    for (int x=x0; x<=x1; x++) { 
        if (steep) { 
            image.set(y, x, color); 
        } else { 
            image.set(x, y, color); 
        } 
        error2 += derror2; 
        if (error2 > dx) { 
            y += (y1>y0?1:-1); 
            error2 -= dx*2; 
        } 
    } 
} 

/**
 * Dessin d'un triangle plein
 */
void triangle(float x0, float y0, float z0, 
                      float x1, float y1, float z1, 
                      float x2, float y2, float z2, 
                      Vec2i uv0, Vec2i uv1, Vec2i uv2, 
                      TGAImage &image, float intensite, 
                      float* zbuffer) {
    
    // Rectangle englobant 
	float minX = std::min(std::min(x0, x1), x2);
	float minY = std::max(std::max(y0, y1), y2);
	float maxX = std::max(std::max(x0, x1), x2);
	float maxY = std::min(std::min(y0, y1), y2);


    float u, v, w, z;
    Vec2i tex;

    // On itère sur le rectangle englobant
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y >= maxY; y--) {
            // On calcul les coordonnées barycentrique
            u = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) / 
                ((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));
            v = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) / 
                ((y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2));
            w = 1 - u - v;

            // Test si le point est à l'intérieur du triangle
            if (u >= 0 && u <= 1 && v >= 0 && v <= 1 && w >= 0 && w <= 1) {
                z = z0 * u + z1 * v + z2 * w;
                tex.x = uv0.x * u + uv1.x * v + uv2.x * w;
                tex.y = uv0.y * u + uv1.y * v + uv2.y * w;

                // Test Z buffer et changement si nécessaire
                int index = x + y * image.get_width();
                if (zbuffer[index] < z) {
                    zbuffer[index] = z;
                    image.set(x, y, TGAColor(intensite * 255, intensite * 255, intensite * 255, 255));
                }
            }
        }
    }
}


int main(int argc, char** argv) {
    if (argc == 2) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head.obj");
    }
    float *zbuffer = new float[width * height];
    std::fill(zbuffer, zbuffer + width * height, -999);
    TGAImage image(width, height, TGAImage::RGB);
    Vec3f light_direction(0, 0, -1);
    // Itération sur les faces du modèle
    for (int i = 0; i < model->nfaces(); i++) {
        std::vector<int> face = model->face(i);
        // Coords des sommets
        Vec3f vertices[3];
        Vec3f original_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f coords = model->vert(face[j]);
            vertices[j] = Vec3f((coords.x + 1.0) * width / 2.0, (coords.y + 1.0) * height / 2.0, coords.z);
            original_coords[j] = coords;
        }
        Vec3f n = (original_coords[2] - original_coords[0]) ^ (original_coords[1] - original_coords[0]);
        n.normalize();
        float light_intensity = n * light_direction;
        if (light_intensity > 0) {
            Vec2i uv[3];
            for (int j = 0; j < 3; j++) {
                uv[j] = model->uv(i, j);
            }
            triangle(vertices[0].x, vertices[0].y, vertices[0].z,
                     vertices[1].x, vertices[1].y, vertices[1].z,
                     vertices[2].x, vertices[2].y, vertices[2].z,
                     uv[0], uv[1], uv[2], image, light_intensity, zbuffer);
        }
    }
    image.flip_vertically();
    image.write_tga_file("output.tga");
    delete[] zbuffer;
    return 0;
}

