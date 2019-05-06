# 3D OpenGL Environment

The goal of this project is to simulate a realistic walk by an avatar, represented by the beloved cartoon character Tigger, walking through an infinitely-tiled world. Some topics explored include mesh loading, VS transformations with 3D camera, texturing, and various shading techniques. 

<p align="center"><img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/banner.png" alt="drawing" width="500"/></p>
<p align="center"><i>This is Tigger staring wistfully into the sunset :'( </i></p>

**Controls**
- `WASD` to move the avatar
- `IKJL` to move the car

## Features
1. **Helicam** - the camera is tied to a helicopter-like, physically simulated (but not displayed) object that is above and behind the avatar, oriented towards the avatar. 
2. **Ground Zero** - infinite ground plane with some tileable texture repeated on it indefinitely.
3. **Pitch Black** - shadows of objects should appear on the ground. These shadows are essentially objects in black, flattened to the ground along a global, non-vertical light direction, but slightly above the ground plane.
4. **Environment Mapping** - the background is reflected on all the objects in the game world.

Env map without environment | Env map with environment |
------------ | ------------- | 
<img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/1.png" alt="drawing" width="400"/> | <img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/2.png" alt="drawing" width="400"/>

5. **Sunshine** - at least one non-vertical directional light source illuminates the game world.
6. **Diffuse** - at least one object has texturing and diffuse (Lambertian) shading.
7. **Shining** - at least one object has specular (diffuse + Phong-Blinn) shading.

Tree with diffuse(3) shading | Tree with specular(4) shading  |
------------ | ------------- | 
<img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/3.png" alt="drawing" width="400"/> | <img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/4.png" alt="drawing" width="400"/>

8. **Spotlight** - at least one point light source fixed to the avatar that moves along with the avatar as it moves and rotates.

Without spotlight | With spotlight  |
------------ | ------------- | 
<img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/5.png" alt="drawing" width="400"/> | <img src="https://github.com/eutopi/3D-OpenGL-Environment/blob/master/Screenshots/6.png" alt="drawing" width="400"/>

9. **Procedural Solid Texturing** - at least one object with procedural solid texturing (marble; see sphere above). The pixel shader computes color from model space position using the following formula.
```
class Marble {
  Marble() {
    scale = 32;
    turbulence = 50;
    period = 32;
    sharpness = 1;
  }
  float3 getColor(float3 position){
  float w = position.x * period 
   + pow(snoise(position * scale), sharpness)*turbulence;

  w = pow(sin(w)*0.5+0.5, 4);
                 // use smooth sine for soft stripes
  return
    float3(0, 0, 1) * w +     // veins
    float3(1, 1, 1) * (1-w);  // rock
  }
};

```

10. **Rolling Ball/Wheel Steering** - spherical, textured objects can roll without sliding. The avatar should be able to push them. The wheels of vehicles also roll as the vehicle move. They are steerable around the vertical axis.
    1. We first obtain the axis of rotation *u* by calculating the normalized cross product of the "ahead" vector (direction the ball/wheel is moving) and the "up" vector (0, 1, 0).
    2. The angle of rotation *theta* is steadily increased by a constant multiple of elapsed time.
    3. We then use Rodrigues' formula to rotate the object by *theta* around *u*.

## Libraries
- OpenGL
- GLUT

## Note
To render each texture properly, must hardcode the paths of each image file.
