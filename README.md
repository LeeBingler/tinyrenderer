# Bresenham's line drawing algorithm

The goal is to create a wireframe renders. To do that we need to draw segment between each point of the model.

The starting point of the project allow us only to draw a individual pixel on a given coordinate with its color: `framebuffer.set(ax, ay, white)`.

To do that we will be using the Bresenham algorithm. But first we will use the simplest approach then optimize our algo to reach the Bresenham algo.

## First attempt, the simplest approach
For the simplest approach, we will simply interpolate a parameter `t` from 0 to 1 between 2 points and draw a pixel at each intermediate value.

We define `t = [0,1]` and the 2D point `x(t), y(t)` :
$$\left\{\begin{array}{l}x(t)= a_x + t \cdot (b_x-a_x) \\y(t)= a_y + t \cdot (b_y-a_y) \\\end{array}\right$$
If t = 0, then `x(t),y(t) = (ax,ay)` and if t = 1 then `x(t),y(t) = (bx,by)`.

__Barycentric coordinates:__
The pair `(1 - t) and t` forms the barycentric coordinates of the number x(t) with respect to the interval `[ax, bx]` and of the number y(t) with respect to the interval `[ay, by]`.

The barycentric coordinates is a way to find the weight of a point inside a shape with weighted point. If the point is in the center then the barycentrci coordinate is 1 and if the point is outside the shape, hen it is negative. That how we do color for fragment shader.

First attempt:
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    for (float t=0.; t<1.; t+=.02) {
        int x = std::round( ax + (bx-ax)*t );
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}
```

Using the simplest approach we see some problems. The pixel is colored dependently of the order of the arguments and there are gaps between pixel.

## Second attempts
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}
```

`x(t) = ax + t * (bx - ax) <=> t(x) = (x - ax) / (bx - ax)`

We defined `t` as a function of `x` allowing us to loop through `x` instead of `t` and solving the gap problem as we have the good amount of pixel drawn.

Using this method 2 new problems shown: vertical steep line have huge gap. If the point c is more on the right then the point b, the segment is not draw (cx > bx).

To solve the cx > bx problem, we need to swap the 2 points if the first point is more on the left than the second point:
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}
```

## Third attempt, bulletproof one
Like the solution for the cx >bx problem, we can transpose (swapping x and y value) to compute  the value of each pixel and de-transpose it when we want to draw the line:
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
    }
}
```

The code is working but it is nor optimize because of the floating-point operation and the division.
Plus, it lack assertion and boundary check.

## Fourth attempt: optimizations
https://haqr.eu/tinyrenderer/bresenham/#fourth-attempt-optimizations

#Warning This sections is where we will transform our simple algo to the Bresenham algorithm. It is not needed to know how it work or how to come up to it.

### **First optimization:**
It is purely mathematical, we change: 
$$y(t)= a_y + t \cdot (b_y-a_y) => y_0 = a_y, y_1 = y_0 + \frac{b_y-a_y}{b_x-a_x}, y_2 = y_1 + \frac{b_y-a_y}{b_x-a_x}$$
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    float y = ay;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        y += (by-ay) / static_cast<float>(bx-ax);
    }
}
```

### **Second optimization:**
In order to change Y from float to int (because pixel can't have decimal).
We introduce an error variable that will track the error commited by `y`. That is, for each value of `x`, `error` gives the distance from `y` to the perfect line segment. Every time `error` exceeds half a pixel, we will increment `y` and decrement `error`.
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    float error = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        error += std::abs(by-ay)/static_cast<float>(bx-ax);
        if (error>.5) {
            y += by > ay ? 1 : -1;
            error -= 1.;
        }
    }
}
```

### **Third optimization**:
We remove intirely the floating and division operation by changing the error variable to an int variable which is equal to `error * 2 * (bx - ax)`. There is no explaination because it's straight up the Bresenham's line-drawing algorithm:
``` cpp
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    int y = ay;
    int ierror = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by-ay);
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx-ax);
        }
    }
}
```

As mentioned earlier, this section serves mainly as a historical tribute to Professor Bresenham. His algorithm is elegant, and the discovery of all-integer rasterization was truly ingenious.

The first optimization is faster than the 3rd one because the `if (ierror > bx - ax)` make a branch making it impossible to pipeline the for loop. Better stick with a float-point algo but without branching as we can now use threading and pipelining.

## Wireframe rendering
In this project we use the wavefornt obj file format. 
To draw the model, we need to read the `v` line (vertices in 3D space) and the first number after each space for the `f` line (faces informations).

We just need to store those 2 data inside an array and then draw line between each vertex inside a triangle to draw the entire model.

# Triangle rasterization

The Goal is to draw 2D filled triangle.

This is the starting point using the line function we build in the previouse lesson:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);
    triangle(  7, 45, 35, 100, 45,  60, framebuffer, red);
    triangle(120, 35, 90,   5, 45, 110, framebuffer, white);
    triangle(115, 83, 80,  90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
```

### Properties of a good triangle-filling algo:
- Should be simple and fast
- It should be symetrical - Should not depend on the order of vertices passed to the function
- Should be no gap between 2 triangles shared vertices

## Scanline rendering, the old school way
[Scanline rendering](https://en.wikipedia.org/wiki/Scanline_rendering)is an algo that will take the top y coordinate, then we draw a segment between most left and most right edge to the bot y coordinate.

1. Sort the vertices of the triangle by their y-coordinates.
2. Rasterize both the left and right edges of the triangle simultaneously.
3. Draw horizontal line segments between the left and right boundary points.

**Rasterize = transform an image / vector object to a pixel grid**

### Sorting the vertices
To sort the vertice, we will simply use a bubble sort and swap the vertices to make the `a` vertice the lowest and the `c` vertice the highest.

``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // sort the vertices, a,b,c in ascending y order (bubblesort yay!)
    if (ay>by) { std::swap(ax, bx); std::swap(ay, by); }
    if (ay>cy) { std::swap(ax, cx); std::swap(ay, cy); }
    if (by>cy) { std::swap(bx, cx); std::swap(by, cy); }
    line(ax, ay, bx, by, framebuffer, green);
    line(bx, by, cx, cy, framebuffer, green);
    line(cx, cy, ax, ay, framebuffer, red);
}
```

### Rasterizing the boundary
The boundary `A` is between `a` and `c`, boundary `B` is between `a` and `b`, and then between `b` and `c`.

So the boundary B is made of two part (Vertice B and the other side of the triangle).

To make things simpler, let's draw only the bottom half edge of the triangle:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // sort the vertices, a,b,c in ascending y order (bubblesort yay!)
    if (ay>by) { std::swap(ax, bx); std::swap(ay, by); }
    if (ay>cy) { std::swap(ax, cx); std::swap(ay, cy); }
    if (by>cy) { std::swap(bx, cx); std::swap(by, cy); }
    int total_height = cy-ay;

    if (ay != by) { // if the bottom half is not degenerate
        int segment_height = by - ay;
        for (int y=ay; y<=by; y++) { // sweep the horizontal line from ay to by
            int x1 = ax + ((cx - ax)*(y - ay)) / total_height;
            int x2 = ax + ((bx - ax)*(y - ay)) / segment_height;
            framebuffer.set(x1, y, red);
            framebuffer.set(x2, y, green);
        }
    }
}
```

### Scanline rasterization

Now we have 2 points with the same y coordinate, we can draw a segment between the 2 points:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // sort the vertices, a,b,c in ascending y order (bubblesort yay!)
    if (ay>by) { std::swap(ax, bx); std::swap(ay, by); }
    if (ay>cy) { std::swap(ax, cx); std::swap(ay, cy); }
    if (by>cy) { std::swap(bx, cx); std::swap(by, cy); }
    int total_height = cy-ay;

    if (ay != by) { // if the bottom half is not degenerate
        int segment_height = by - ay;
        for (int y=ay; y<=by; y++) { // sweep the horizontal line from ay to by
            int x1 = ax + ((cx - ax)*(y - ay)) / total_height;
            int x2 = ax + ((bx - ax)*(y - ay)) / segment_height;

            // draw a horizontal line
            for (int x=std::min(x1,x2); x<std::max(x1,x2); x++)
                framebuffer.set(x, y, color);
        }
    }
}
```
We can also use the line function for this.

We can now draw the upper half of the triangle using another loop:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    // sort the vertices, a,b,c in ascending y order (bubblesort yay!)
    if (ay>by) { std::swap(ax, bx); std::swap(ay, by); }
    if (ay>cy) { std::swap(ax, cx); std::swap(ay, cy); }
    if (by>cy) { std::swap(bx, cx); std::swap(by, cy); }
    int total_height = cy-ay;

    if (ay != by) { // if the bottom half is not degenerate
        int segment_height = by - ay;
        for (int y=ay; y<=by; y++) { // sweep the horizontal line from ay to by
            int x1 = ax + ((cx - ax)*(y - ay)) / total_height;
            int x2 = ax + ((bx - ax)*(y - ay)) / segment_height;
            for (int x=std::min(x1,x2); x<std::max(x1,x2); x++)  // draw a horizontal line
                framebuffer.set(x, y, color);
        }
    }
    if (by != cy) { // if the upper half is not degenerate
        int segment_height = cy - by;
        for (int y=by; y<=cy; y++) { // sweep the horizontal line from by to cy
            int x1 = ax + ((cx - ax)*(y - ay)) / total_height;
            int x2 = bx + ((cx - bx)*(y - by)) / segment_height;
            for (int x=std::min(x1,x2); x<std::max(x1,x2); x++)  // draw a horizontal line
                framebuffer.set(x, y, color);
        }
    }
}
```

## Modern rasterization approach

The code we've made is design for a signle-threaded CPU programming and is not optimize for our modern PC.

We will use this pseudo-code to guide us as the modern approach is litteraly this:
``` python
triangle(vec2 points[3]) {
    vec2 bbox[2] = find_bounding_box(points);
    for (each pixel in the bounding box) {
        if (inside(points, pixel)) {
            put_pixel(pixel);
        }
    }
}
```

### Find Bounding Box
First we need to get the bounding box. It's easy we loop through the vertices and take the min and max for x and y. With that we have a top left corner and the right bottom corner of the bounding box. Then we draw the bb using multy threading:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
#pragma omp parallel for
    for (int x=bbminx; x<=bbmaxx; x++) {
        for (int y=bbminy; y<=bbmaxy; y++) {
            framebuffer.set(x, y, color);
        }
    }
}
```
Note the line 6. Once we have determined the bounding box, this line tells to run the loop **in parallel**. As opposed to the scanline method, here each pixel is treated separately, and thus this is an [embarrassingly parallel](https://en.wikipedia.org/wiki/Embarrassingly_parallel#:~:text=In%20parallel%20computing%2C%20an%20embarrassingly,a%20number%20of%20parallel%20tasks) method.

### Check pixel in triangle

To do that, we will compute the barycentric coordinates of the pixel `P` inside the triangle `ABC`.

We can express `P` as a combination of the triangle's vertices: $$ P=\alpha A + \beta B + \gamma C$$While being a bit frightening at the first glance, it is really simple: imagine that we put three weights at the vertices `A`,`B`, and `C`. Then the barycenter of the system is exactly in the point `P`.

Barycentric coordinates are proportional to sub-triangle areas `Area(PBC)` ,`Area(PCA)` and `Area(PAB)`. If all three coordinates are positive, `P` lies within the triangle. If any barycentric coordinate is negative, `P` lies outside the triangle.


To put it simply, P is equal to the area of PAB, PBC and PCA each divided by the total Area of the triangle ABC. And if the result of one of the area is less than 0, then the pixel P is outside the triangle.

The weight is in fact the area between 2 points of the triangle + the pixel P divided by the area of the triangle.

In code:
``` cpp
double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5*((by-ay)*(bx+ax) + (cy-by)*(cx+bx) + (ay-cy)*(ax+cx));
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

#pragma omp parallel for
    for (int x=bbminx; x<=bbmaxx; x++) {
        for (int y=bbminy; y<=bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha<0 || beta<0 || gamma<0) continue; // negative barycentric coordinate => the pixel is outside the triangle
            framebuffer.set(x, y, color);
        }
    }
}
```

## Putting all together + back-face culling

From the previous lesson, we will take the excat sime code but instead of calling the line function 3 times to draw the wireframe, we will call the triangle function in order to draw each face:
``` cpp
// First of all, (x,y) is an orthogonal projection of the vector (x,y,z).
// Second, since the input models are scaled to have fit in the [-1,1]^3 world coordinates,
// we want to shift the vector (x,y) and then scale it to span the entire screen.
std::tuple<int,int> project(vec3 v) { 
    return { (v.x + 1.) *  width/2,
             (v.y + 1.) * height/2 };
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    for (int i=0; i<model.nfaces(); i++) { // iterate through all triangles
        auto [ax, ay] = project(model.vert(i, 0));
        auto [bx, by] = project(model.vert(i, 1));
        auto [cx, cy] = project(model.vert(i, 2));
        TGAColor rnd;
        for (int c=0; c<3; c++) rnd[c] = std::rand()%255;
        triangle(ax, ay, bx, by, cx, cy, framebuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
```

But with this code, the back face are also drawn and we dont have any z-index to tell us which face is drawn on top of the other.

To address this issue, we will simply add one line of code at line 7:
``` cpp
void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    
    // backface culling + discarding triangles that cover less than a pixel
	if (total_area<1) return;

#pragma omp parallel for
    for (int x=bbminx; x<=bbmaxx; x++) {
        for (int y=bbminy; y<=bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha<0 || beta<0 || gamma<0) continue; // negative barycentric coordinate => the pixel is outside the triangle
            framebuffer.set(x, y, color);
        }
    }
}
```

If the area is 1 pixel, we don't need to draw it.

If the area is negative, it means that the triangle is draw from right to left and so that it is facing the back of the model.

# Barycentric coordinates

## 1D barycentric coordinates
A barycentric coordinates for a 1D segment can be represented as:
$$P = \alpha A + \beta B$$
If we place a weight of α kg to the point and β kg to the point , then the barycenter of the system will be situated in the point P. Note that the barycenter would not move if we put weights `13α` and `13β` instead of `α` and `β`. The usual way to eliminate this redundancy is to distribute 1kg in total:
$$\alpha + \beta = 1.$$

To define `α` and `β` We have those 2 equations from above and if we solve them we got:
$$\alpha = \frac{B - P}{B - A}, \quad \beta = 1-\alpha = \frac{P - A}{B - A}.$$

Example:
![](https://haqr.eu/tinyrenderer/barycentric/1d.png)

The more the point P is near a point, the greater is the weight but only if the point P is between A and B.

A weight can be negative, meaning it can pull up the rod instead of weigh down. If it's the case, it means that the point P is out of the segment AB.

#Important
Let us sum up:
- If P = A, then `α` = 1 and `β` = 0 .
- If P = B, then `α` = 0 and `β` = 1.
- If both coordinates are **strictly between 0 and 1**, is strictly inside the segment.
- If P is outside `[A,B]` , then one coordinate is **negative**.

## 2D barycentric coordinates

The principle is the same but we had another point to make a triangle in order to compute the barycenter coordinate in 2D space instead of 1D space:
$$P = \alpha A + \beta B + \gamma C$$
And :
$$\alpha + \beta + \gamma = 1.$$
We compute the weight of the point P the same way as for the 1D exemple but we had another equation to the party to be able to get the weight for x-axis and y-axis:
$$\left\{\begin{array}{ll}\alpha A_x + \beta B_x + \gamma C_x &= P_x\\\alpha A_y + \beta B_y + \gamma C_y &= P_y\\\alpha + \beta + \gamma &= 1\end{array}\right.$$
### (Optional) how to solve this system 
In order to solve this system we will use a matrix. (I won't be covering that as it is difficult and we won't be using that for the our coordinate)

### Pratical computation
With the mathematical that I didnt cover, we see that the weight of each point is equal to:
$$\alpha = \frac{ \, \text{Area}(PBC) \, }{ \, \text{Area}(ABC) \, }.$$
Similarly:
$$\beta = \frac{ \, \text{Area}(PCA) \, }{ \, \text{Area}(ABC) \, }, \quad \gamma = \frac{ \, \text{Area}(PAB) \, }{ \, \text{Area}(ABC) \, }.$$


To compute the area of each triangle, we will be using the [shoelace formula](https://en.wikipedia.org/wiki/Shoelace_formula), giving us this for our triangle:
$$\text{Area}(ABC) = \frac{1}{2} \left( \small(B_y-A_y)(B_x+A_x) + (C_y-B_y)(C_x+B_x) + (A_y-C_y)(A_x+C_x) \right).$$

The order of the point matter, as the formula gives the signed area.

#Important 
The interpretation of the weights is very similar to the 1D case:
- If `α`,`β`, `γ`  are all **between 0 and 1**, P is **inside** the triangle.
- If any coordinate is **negative**, P is **outside** the triangle.
- If one coordinate is exactly **zero**, P lies **on an edge**.
- If two coordinates are **zero**, P is exactly on a vertex.

### Example:
![](https://haqr.eu/tinyrenderer/barycentric/2d_b.png)

I chose A,B,C points to have(16,0), (0,12), (0, 0) coordinates , respectively. 
Then I took point P = (4,3).
We then use the shoelace formula on each triangle and the main triangle.
$$\text{Area}(ABC)=\frac12\begin{vmatrix}16 & 0  &0\\0  & 12 &0\\1 & 1  &1\end{vmatrix} = 96,\quad\text{Area}(PBC)=\frac12\begin{vmatrix}4 & 0 &0\\3 & 12 &0\\1 & 1  &1\end{vmatrix} = 24$$
$$\text{Area}(PCA)=\frac12\begin{vmatrix}4 & 0 & 16\\3 & 0 & 0 \\1 & 1 & 1\end{vmatrix} = 24,\quad\text{Area}(PAB)=\frac12\begin{vmatrix} 4 & 16 & 0\\ 3 & 0  & 12 \\ 1 & 1  &1\end{vmatrix} = 48$$
By dividing these areas by the area of ABC, we obtain barycentric coordinates of the point P:
$$\alpha=\frac14,\quad \beta=\frac14,\quad\gamma=\frac12$$

Btw, the weight is attributed by the vector you left over, for the PCA area you are computing the weight Beta as B is left out. For PAB its Gamma as C is left out. And for PBC its alpha as A is left out.

# Hidden faces removal

The goal of this lesson is to draw the triangle in the right z-axis and eliminate the hidden surface.

The idea is to put this pseudo code in real c++ code:
```
for each triangle t:
    if t is not back-facing:
        for each pixel p that t covers:
            paint the pixel
```

## Painter's algorithm

The painter's algorithm consist of painting the model from rear-to-front order by sorting the triangles by depth. It is not used today as we want to not render triangle that we don't see.

Here is the painter's algo pseudo code:
```
sort triangles by depth
for each triangle t:
    for each pixel p that t covers:
        paint the pixel
```

We don't use this algo because for each camera movement and model movement, we need to re-sort the entire scene.

Plus, triangle are not the best primitive for global ordering. Instead we will handle this per pixel.

## Depth interpolation

Depth interpolation consist of computing the depth z of the pixel, putting it in a grayscale image / buffer and then drawing the pixel only if the depth z value is greater than the one in the buffer.

Here is the pseudo code:
```
for each triangle t:
    for each pixel p that t covers:
        compute its depth z
		if depth buffer at p < z:
		    update the depth buffer with z
		    paint the pixel
```

Here is what the grayscale buffer should be:
[![](https://haqr.eu/tinyrenderer/z-buffer/diablo-zbuffer.png)](https://haqr.eu/tinyrenderer/z-buffer/diablo-zbuffer.png)

# Naive Camera Handling

This is a Camera handling only to understand how basic projection works. Wiat for the next lesson to implement a camera like real renderer.

## Rotating the camera

For now, our camera is an orthographic camera; Regardless of the depth of the model (z-zxis), the model always have the same size.

To simulate a camera, we move the entire scene rather than moving the camera itself. Instead of rotating the camera to the left, we rotate the scene to the right.

Be sure to know how vector and matrix math work before doing any of this, as it is important for 3D graphic.

## Central projection

Also known as perspective projection, it offer more realism to the scene.

As we are projecting a 3D scne in 2D, the nearer a face is, the bigger we need to make it. To do that, we will project near point to the plane we are rendering.

Here is a better explanation:

![](https://haqr.eu/tinyrenderer/camera-naive/persp2-a.svg)

We want to project the near point to the plan J which is the plane we will draw on the screen.

For that we will use the Thales theorem to project the near point to the J plane.

Knowing the camera point C = (10,0). We can project our point using this calcul:
$$\frac{(x, y)}{1 - \frac{x}{C}}.$$

Resulting in our new coordinate:
$$\begin{array}{l}\frac{(2, 0)}{1 - \frac{2}{10}} = \left(\frac{5}{2}, 0\right) \\\frac{(0, 2)}{1 - 0} = (0, 2) \\\frac{(-2, 2)}{1 + \frac{2}{10}} = \left(-\frac{5}{3}, \frac{5}{3}\right) \\\frac{(-2, -2)}{1 + \frac{2}{10}} = \left(-\frac{5}{3}, -\frac{5}{3}\right) \\\frac{(2, -2)}{1 - \frac{2}{10}} = \left(\frac{5}{2}, -\frac{5}{2}\right)\end{array}$$

Here is the deformed object that will be render:
![](https://haqr.eu/tinyrenderer/camera-naive/persp2-b.svg)

After deforming the object, we can apply the orthographic projection.

# Better camera handling

This is the way that all renderer handle camera and transformation by rendering pipeline.
See this 2 articles to go further :
- https://learnopengl.com/Getting-started/Transformations
- https://learnopengl.com/Getting-started/Coordinate-Systems
![](https://learnopengl.com/img/getting-started/coordinate_systems.png)

## Chain of coordinate transformation

The transformation chain for renderer (rendering pipeline) is this:
1. **Local Space** - Objects are modeled in their own local frame (object coordinates)
2. **World Space** - Objects are place into a scene (world coordinates)
3. **View Space** - The scene is expressed in the camera frame (eye coordinates)
4. **Clip Space** - We deform the scene to apply perspective (clip coordinates [-1, 1])
5. **Screen Space** - We render on screen by converting clip coordinates to screen coordinates

The goal is to position, orient and scale object in a scene using composable transformation (model matrix, view matrix and perpective matrix).

To do that we will need to do some math on the coordinates of each point. As everything in a 3D scene is a (x,y,z) point or vector.

## Linear transformations

To understand linear transformation, see this video: https://www.youtube.com/watch?v=kYB8IZa5AuE

The article is good but the video is just better to understand.

Basically, the orthogonal 2D plan is mesure with x = (1,0) and y = (0,1) (cartesian coordinate)
If we change how we mesure the plan, we change the plan itself.

If x become (0, -1) and y become (1, 0), then the whole plan will be turn by 90°. Because every point inside a plan is define by (x,y) and x is a mesure that can be change, then eveything can be change.

To apply this linear transformation, we just multiply the matrix by a vector:
$$\begin{bmatrix}a&b\\c&d\end{bmatrix}\begin{bmatrix}x\\y\end{bmatrix} = \begin{bmatrix}ax+by\\cx+dy\end{bmatrix}.$$
The matrix will represent (x,y) for this vector with x being the 1st row and y the 2nd.

## Affine transformation

Affine transformation allow us to move (translate) point by a vector. We just need to add a vector to the coordinate of the point:
$$\begin{bmatrix}x\\y\end{bmatrix} \quad \mapsto \quad \begin{bmatrix}a&b\\c&d\end{bmatrix}\begin{bmatrix}x\\y\end{bmatrix} + \begin{bmatrix}e\\f\end{bmatrix} = \begin{bmatrix}ax+by+e\\cx+dy+f\end{bmatrix}$$
The matrix abcd is here only to show us that in order to do the affine and the linear transformation, we need to do 2 calculus and it become heavier each time we want to do more (like 5 time for the rendering pipeline).

If we want to do 2 time the linear + affine transformation, the caculation become this:
$$\begin{bmatrix}x\\y\end{bmatrix} \quad \mapsto \quad \begin{bmatrix}a_2&b_2\\c_2&d_2\end{bmatrix}
\left(\begin{bmatrix}a_1&b_1\\c_1&d_1\end{bmatrix}\begin{bmatrix}x\\y\end{bmatrix} + \begin{bmatrix}e_1\\f_1\end{bmatrix}\right)+\begin{bmatrix}e_2\\f_2\end{bmatrix}$$

We need to make it simpler for us and for the computer to be efficient
## Homogeneous Coordinates

To do the Linear and the Affine transformation at the same time instead of doing it in two time (like the calculation above), we can add a columns to the matrix to put the affine transformation in this columns and doing everything in one matrix:
$$\begin{bmatrix}a&b&e\\c&d&f\\0&0&1\end{bmatrix}\begin{bmatrix}x\\y\\1\end{bmatrix} = \begin{bmatrix}ax+by+e\\cx+dy+f\\1\end{bmatrix}$$

If we multiply this matrix and the vector augmented by 1 we get another vector with 1 in the last component, but the other two components have exactly the shape we would like!

That's why we often use 3x3 matrix in 2D and 4x4 in 3D graphics.

In fact, the idea is really simple. Translations are not linear in the 2D space. So we embed our 2D into 3D space (by simply adding 1 for the 3rd component). It means that our 2D space is the plane `Z = 1` in the 3D space. Then we perform a linear 3D transformation and project the result onto our 2D physical plane. How do we project 3D back onto the 2D plane? Simply by dividing by the third component:
$$\begin{bmatrix}x\\ y\\ z\end{bmatrix} \quad \mapsto \quad \begin{bmatrix}x/z \\ y/z\end{bmatrix}$$

Since z is always 1, we can just take x and y. But try not to do that as Z will not always be 1.

While being strange at the first sight, the approach is very straightforward, and it offers two serious advantages:
1. **Translation becomes matrix multiplication.** Affine transformations (like translation) are not linear in Cartesian coordinates but are linear in homogeneous coordinates, thus allowing for the composability we were looking for.
2. **Enables perspective projection.** Homogeneous coordinates naturally handle perspective division, which is key for realistic rendering of 3D scenes (we will revisit it shortly). Therefore, all camera handling (rotation, scaling, translation, projection) can be done within the same consistent framework.

## Putting it all together

In previous lesson, we chained 3 differents transformations:
``` cpp
auto [ax, ay, az] = project(persp(rot(model.vert(i, 0))));
```

Now we want to do it with only matrix to handle it all in the same place:
``` cpp
mat<4,4> ModelView, Viewport, Perspective;
```

### Viewport

Replace project function.

The viewport transform scale the scene to the size of the screen (from clip space to screen space).

Since clip is between -1 and 1 and pixel can't be negative, we need to add 1 to each vertex.
And as we want the range to transform from [0, 2] to [0, width], we need to multiply each vertex by width / 2.

We finish with this:
```
v.x + 1 = 0 → 0 * width/2 = 0 
v.x + 1 = 1 → 1 * width/2 = width/2 (the center) 
v.x + 1 = 2 → 2 * width/2 = width (the right edge)
```

So if we take a point P with (x,y) coordinate, we end up with this matrix:
$$\begin{bmatrix}\frac{width}{2}&0&0&x+\frac{width}{2} \\ 0&\frac{height}{2}&0&y+\frac{height}{2}\\ 0&0&1&0\\ 0 &0 &0&1\end{bmatrix}$$

We end up with this code:
``` cpp
void viewport(const int x, const int y, const int w, const int h) {
    Viewport = {{
	    {w/2., 0   , 0, x+w/2.}, 
	    {0   , h/2., 0, y+h/2.}, 
	    {0   , 0   , 1, 0     }, 
	    {0   , 0   , 0, 1     }
    }};
}
```

### Perspective deformation

Replace persp function.

We seen in the previous lesson [[#Central projection]] how to do a perspective deformation:
``` cpp
vec3 persp(vec3 v) {
    constexpr double f = 3.;
    return v / (1-v.z/f);
}
```

We need to change it to a matrix in homogenous coordinates.

For a 3D point (x,y,z), we embed it into 4D by adding 1 as the last coordinate (x,y,z,1): . Next, we multiply it with the perspective matrix, getting again a 4D point. Finally, we re-project (do not confond with the projection to the screen) the result to 3D by dividing by the last coordinate. Here is the computation flow for the above function:
$$\begin{bmatrix}v_x \\ v_y \\ v_z \end{bmatrix} \quad \mapsto \quad\begin{bmatrix}1&0&0&0 \\ 0&1&0&0\\ 0&0&1&0\\ 0 &0 &-\frac{1}{f}&1\end{bmatrix}\begin{bmatrix}v_x \\ v_y \\ v_z \\ 1\end{bmatrix}=\begin{bmatrix}v_x \\ v_y \\ v_z \\ 1-\frac{v_z}{f}\end{bmatrix}\quad \mapsto \quad \frac{1}{ 1-\frac{v_z}{f}}\begin{bmatrix}v_x \\ v_y \\ v_z \end{bmatrix}$$
To setup the perspective matrix we just need to do this:
``` cpp
void perspective(const double f) {
    Perspective = {{{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1/f,1}}};
}
```

### ModelView
ModelView matrix is the matrix that change the world space to view space.

Just read the article, i'm not going to write all this math for fun: https://haqr.eu/tinyrenderer/camera/#modelview

To do that, we will be replicated the glulookAt function from OpenGL Utility Library.

For that we need 3 view parameters:
1. The 3D point `eye` - defines the location of the camera
2. The 3D point `center` - denotes the direction the camera is aiming at.
3. The vector up - denotes the roughly upward orientation of the camera

![](https://haqr.eu/tinyrenderer/camera/glulookat.svg)

It means that we want to do the rendering in the frame (center,l,m,n), where l, m and n are computed from the parameters eye, center and up.

#### Step 1 - Build camera origin
N is the backward axis (z-axis) so it's always toward the eye
L is the "right" axis, since we have the up vecto, we can do a cross product to find it.
M is the "true up" axis, since we know N and L, we can do a cross product to find it.

BTW, thecross product find the orthogonal axis to 2 vectors.

#### Step 2 - change of basis (from xyz to lmn)
To get a point P in view space, we just need this:
![](https://haqr.eu/tinyrenderer/camera/basis_change.svg)
$$\overrightarrow{OP} = \overrightarrow{OC} + \overrightarrow{CP} = \begin{bmatrix}\vec{i} & \vec{j} & \vec{k}\end{bmatrix}\begin{bmatrix}C_x \\ C_y \\ C_z\end{bmatrix} + \begin{bmatrix}\vec{l} & \vec{m} & \vec{n}\end{bmatrix} \begin{bmatrix}x' \\ y' \\ z'\end{bmatrix}$$

To translate a pixel P to the good cordinate, we need to move it using the vector OP.
And OP is OC in (i,j,k) coordinate + CP in (l,m,n) coordinate.

After some math, we end up with this:
$$\begin{bmatrix}x' \\ y' \\ z'\end{bmatrix}  =M^{-1}\left( \begin{bmatrix}x \\ y \\ z\end{bmatrix} - \begin{bmatrix}C_x \\ C_y \\ C_z\end{bmatrix}\right).$$
Where M is the matrix whose columns are l, m, n (expressed in world coordinates).

Two things make this simple in practice:
1. **Subtract C first** — this re-centers the point relative to the camera's origin (`center`), i.e. it's a _translation_.
2. **Multiply by M⁻¹** — this re-expresses the now-shifted point in the new basis, i.e. it's a _rotation_.

And because (l,m,n) is orthonormal (unit length, mutually perpendicular), M-1 is the transpose of M.
Much cheaper to compute.

$$M = \begin{bmatrix}\vec{l}_x & \vec{m}_x & \vec{n}_x \\ \vec{l}_y & \vec{m}_y & \vec{n}_y \\ \vec{l}_z & \vec{m}_z & \vec{n}_z \end{bmatrix} => M^{-1} = M^\top = \begin{bmatrix}\vec{l}_x & \vec{l}_y & \vec{l}_z \\ \vec{m}_x & \vec{m}_y & \vec{m}_z \\ \vec{n}_x & \vec{n}_y & \vec{n}_z \end{bmatrix}$$
#### Step 3 - Assemble the 4x4 ModelView matrix

Using homogenous coordinates, transkation and rotation combine into one matrix product:
$$\begin{bmatrix}x' \\ y' \\ z' \\ 1\end{bmatrix}  =\underbrace{\begin{bmatrix}\vec{l}_x & \vec{l}_y & \vec{l}_z & 0\\ \vec{m}_x & \vec{m}_y & \vec{m}_z & 0 \\ \vec{n}_x & \vec{n}_y & \vec{n}_z & 0 \\ 0 & 0 & 0 & 1\end{bmatrix}\begin{bmatrix}1 & 0 & 0 & -C_x\\ 0 & 1 & 0 & -C_y \\ 0 & 0 & 1 & -C_z \\ 0 & 0 & 0 & 1\end{bmatrix}}_{\text{ModelView}} \begin{bmatrix}x \\ y \\ z \\ 1\end{bmatrix}.$$
The matrix on the left is the M-1 matrix and the matrix on the middle is the -C translation to go from model origin to center origin.

Now we just need to multiply any coordinate by the ModelView to get the coordinate in the camera frame.

The final code is this:
``` cpp
void lookat(const vec3 eye, const vec3 center, const vec3 up) {
    vec3 n = normalized(eye-center);
    vec3 l = normalized(cross(up,n));
    vec3 m = normalized(cross(n, l));
    ModelView = mat<4,4>{{{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}}} *
                mat<4,4>{{{1,0,0,-center.x}, {0,1,0,-center.y}, {0,0,1,-center.z}, {0,0,0,1}}};
}
```

Still need to understand the last commit from this lesson:
``` cpp
void rasterize(const vec4 clip[3], std::vector<double> &zbuffer, TGAImage &framebuffer, const TGAColor color) {
	// normalized device coordinates (vec4 to vec3)
    vec4 ndc[3]    = { clip[0]/clip[0].w, clip[1]/clip[1].w, clip[2]/clip[2].w };      // screen coordinates (vec3 to vec2)
    vec2 screen[3] = { (Viewport*ndc[0]).xy(), (Viewport*ndc[1]).xy(), (Viewport*ndc[2]).xy() }; 
	
	// Barycebtric coordinate of triangle in vec2
    mat<3,3> ABC = {{ {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} }};

    // backface culling + discarding triangles that cover less than a pixel
    if (ABC.det()<1) return; 

	// bounding box for the triangle
    auto [bbminx,bbmaxx] = std::minmax({screen[0].x, screen[1].x, screen[2].x}); 
    auto [bbminy,bbmaxy] = std::minmax({screen[0].y, screen[1].y, screen[2].y}); 
#pragma omp parallel for
    for (int x=std::max<int>(bbminx, 0); x<=std::min<int>(bbmaxx, framebuffer.width()-1); x++) { // clip the bounding box by the screen
        for (int y=std::max<int>(bbminy, 0); y<=std::min<int>(bbmaxy, framebuffer.height()-1); y++) {
	        // barycentric coordinates of Point in the triangle
            vec3 bc = ABC.invert_transpose() * vec3{static_cast<double>(x), static_cast<double>(y), 1.}; 

            // negative barycentric coordinate => the pixel is outside the triangle
            if (bc.x<0 || bc.y<0 || bc.z<0) continue;

            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x+y*framebuffer.width()]) continue;
            zbuffer[x+y*framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}
```

Need to understand the code.
Especially ABC matrix and bc = ABC.invert_transpose()

#### The ABC matrix — this is barycentric coordinates in matrix form

```cpp
mat<3,3> ABC = {{ {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} }};
```

To check whether a point P = (x, y) is inside triangle ABC and to get its weights (α, β, γ), you want to solve:

$$\alpha A + \beta B + \gamma C = P, \quad \alpha+\beta+\gamma = 1$$

If you write A, B, C, and P in the augmented (homogeneous 2D) form `(x, y, 1)`, this whole system becomes **one matrix equation**:

$$\begin{bmatrix}A_x & B_x & C_x\\ A_y & B_y & C_y\\ 1 & 1 & 1\end{bmatrix}\begin{bmatrix}\alpha\\\beta\\\gamma\end{bmatrix} = \begin{bmatrix}P_x\\P_y\\1\end{bmatrix}$$​​

That "1" row automatically encodes the constraint α+β+γ=1 — exactly the constraint you saw in [[#Example]]. Notice the matrix on the left has **columns** = A, B, C (each augmented with a 1).

Now look at how `ABC` is actually built in code: its **rows** are `{Ax,Ay,1}`, `{Bx,By,1}`, `{Cx,Cy,1}`. That means `ABC` is the **transpose** of the matrix in the equation above. So the equation really being solved is:

$$\text{ABC}^\top \begin{bmatrix}\alpha\\\beta\\\gamma\end{bmatrix} = \begin{bmatrix}P_x\\P_y\\1\end{bmatrix} \quad\Longrightarrow\quad \begin{bmatrix}\alpha\\\beta\\\gamma\end{bmatrix} = (\text{ABC}^\top)^{-1}\begin{bmatrix}P_x\\P_y\\1\end{bmatrix}$$​​

And `(ABC⊤)−1 = (ABC−1)⊤` — the transpose of the inverse. That's precisely what `.invert_transpose()` computes. So this line, later in the pixel loop:


```cpp
vec3 bc = ABC.invert_transpose() * vec3{x, y, 1.};
```

directly gives you `bc = (α, β, γ)` for the pixel `(x, y)` — no determinants needed per-pixel, because the whole "inverse" is precomputed **once per triangle**, and every pixel just does one cheap matrix-vector multiply.

This is a much faster version of what you did by hand in [[#Example]] (computing three separate sub-triangle areas for every query point) — the matrix inversion bakes all of that into one reusable operator for the whole triangle.

# Shading

The goal of this lesson is to shade hte model with Phong Technique

## Rendering pipeline

![](https://haqr.eu/tinyrenderer/shading/pipeline.svg)

It consists of the following main stages:
- **Vertex processing**: processes individual vertices. (Vertex Shader)
- **Primitive assembly**: connects vertices to form primitives.
- **Rasterization**: converts each primitive into a set of fragments.
- **Fragment processing**: processes individual fragments. (Fragment shader)
- **Output merging**: combines the fragments of all primitives (in 3D space) into a 2D color pixel for the display.

Chain of coordinate transformation happend in the Primitive assembly. Right before rasterizing.

We refactor the code to match the rendering pipeline (I won't copy-paste all the code here)

## Phong reflection model

FUN FACT: Programmable stages of the rendering pipeline are called shaders because approximation of the illumination is their main purpose.

The Phong reflection is th sum of three component:
- The **ambient term** models indirect light — the constant background illumination present even in shadowed areas.
- The **diffuse term** represents light scattered equally in all directions from rough surfaces.
- The **specular term** simulates bright highlights on shiny surfaces caused by mirror-like reflection.

If we store each term in its own image, we can obtain the final result as a simple (weighted) sum:
![](https://haqr.eu/tinyrenderer/shading/phong-reflectance.png)

### The practical recipe

#### Ambient term
It's a constant fill light, it doesn't depends on geometry so we can just say its 0.3 if we want.

#### Diffuse (Lambert) term

It models matte scattering, i.e. received energy depends on how much the surface faces the light.

The more the surface is align with the light, the brigther it get

To check how much the surface is bright, we need to:
1. Calculate the angle α between the normal (vector orthogonal to the triangle) and the light.
2. Check how much there are align to return the brightness

To calculate the normal, we just need to get the cross product of the edge:
$$\vec n=\frac{\overrightarrow{AB} \times \overrightarrow{AC} }{||\overrightarrow{AB} \times \overrightarrow{AC}||}.$$

Then we can get the angle α between the vector l (toward the light) and the normal with this:
$$\cos \alpha = \vec n \cdot \vec l$$

Here is an illustration:
![](https://haqr.eu/tinyrenderer/shading/vectors1.svg)



#### Specular term

It simulates the bright highlights on shiny surface caused by mirror reflection.

The more the viewer is align with the reflection of the light on the surface, the brighter the highlight get.

To check the brightness, we need to:
1. Do the same step as the Diffuse term (find normal + calculate angle α)
2. Calculate the angle of the reflection
3. Finally, Calculate the angle `β` between the viewer and the reflection to determine the brigthness

To find the vector r (the reflection of the light) with vector n and vector l that are normalized, we can do:
$$ \vec r = 2\vec n (\vec n \cdot \vec l ) - \vec l $$

Then we can just calculate the angle `β` like we calculate the angle α but with the viewer vector and the reflection vector:
$$\cos \beta = \vec r \cdot \vec v$$

A perfect mirror appears brightest if the viewer direction `v` exactly matched `r`. For non-perfectly-polished materials, the highlight should fade away as moves `v` away from `r`.

To do this trick we will raise the cosine to a shininess exponent `e`.

The specular term can be found as:
$$(\max(0, \cos \beta))^e$$

The more `e` is large, the more the highlight will be tight and bright.



# More Data! (texture)
https://haqr.eu/tinyrenderer/textures/
## Smooth shading
To make the triangle smoother instead of flat we need to interpolate the pixel by the vector of the triangle.

To do that we use the normal vector on each vector and we use the cubic Hermit spline making the pixel inside the triangle curve.

<img width="1598" height="584" alt="curved-triangle" src="https://github.com/user-attachments/assets/14030eb9-72ec-4d81-92f3-52fc9e0c383b" />


In the .obj format, the normal vector start with `vn`
The `f` line (face), the last digit of the triplet represent the indice of the normal vector.

### cheating
Instead of computing the barycentric interpolation, we will interpolate the normal vector directly. Giving us varying shading across triangle. This will save us the computation of the barycentric interpolation which was expensive.

## Textures
We use image to add more data to the obj and make it more realistic.

But in order to use those image we need to get the UVs of the object. UVs are 2D coordinate that match each 3D vertex in order to get values from a image. The idea is to superpose the flattened 2D mesh onto an image.

With those UVs we can get a color, a normal value and more for each pixel on the object.

<img width="1772" height="1139" alt="textures" src="https://github.com/user-attachments/assets/e84bc041-c2a4-47c7-8047-6fe818508eae" />

# Tangent  space normal mapping
## Global vs tangent space
Normal mapping is a technique to have a normal value per pixel instead of per vertex without increasing the complexity of the model. We generally store the normal map as an image.

But we have 2 type of normal map, the global-space normal (left) and the tangent-space normal (right):
<img width="1024" height="512" alt="diablo-nm" src="https://github.com/user-attachments/assets/c89a45f7-b381-476e-ae70-6b2b43b2b5fb" />

PROBLEM: Using global-space normal is bad because we can use the same normal UV for multiple part of the model. (left arm have the same normal as the right arm because they are the same).
So the normal point in the same direction for the right and left arm but shouldn't.

SOLUTION: Stop encoding the absolute value of the normal for each texel and using a normal value relative to the local surface orientation at that point.
Then the same texel color means "perturb the surface's own local normal by this much," which is correct regardless of how that triangle happens to be rotated/mirrored/positioned in UV space, because you reconstruct the final world-space normal on the fly using each triangle's own local frame.

The idea is that from the base of a texel, we draw a vector perpendicular from it. Then with the information from the tangent-space normal, we know how much we should move the perpendicular vector to get a normal vector.

It solve the problem cause the part that use multiple time the same UVs will not get the same normal as they don't have the same perpendicular vector at the start of the normal computation.

To find the local point for the perpendicular vector, we draw a grid on the model and on each intersection of the blue and red line we make a normal vector:
<img width="1670" height="1085" alt="head-tbn" src="https://github.com/user-attachments/assets/fa451b37-38ea-40ef-8c8a-5bf52d841955" />

## Computing tangent frame
To compute the tangent frame, for each vector we need 3 data:
- N: The surface normal (the straight up vector from the base of the point)
- T: The tangent (points along the surface in the direction the U-coordinate increases)
- B: The bitangent (points along the surface in the direction the V-coordinate increases; typically B=N×TB = N \times T B=N×T)

# Shadow mapping

Shadow mapping is a technique to get hard shadow on surface without doing all the ray-tracing BS.

The idea is to use the depth buffer 2 times :
1. **First pass (from the light’s perspective):** The scene is rendered from the light’s point of view. The depth buffer stores the distance of the closest surfaces to the light. This buffer (the shadow map) encodes which parts of the scene are visible to the light source.
2. **Second pass (from the camera’s perspective):** The scene is rendered normally from the viewer’s position, but for each fragment we perform an extra check: we test whether the fragment is visible to the light or hidden in shadow.

Here is a schematic view of the two spaces involved:
<img width="2135" height="688" alt="spaces" src="https://github.com/user-attachments/assets/0250d529-cf40-4bbb-aa08-7c933cb1b4d7" />


To link the two passes, we keep track of both `Viewport * Perspective * ModelView` products. This allows us to transform any fragment seen from the camera back into the light’s screen space.

Let us say we have a fragment `f = (i, j, z, 1)` in the camera’s screen-space coordinates. We can transform it back to object coordinates using `M^-1 f`, and then into the light’s screen-space coordinates using `NM^-1 f`, where is the camera transform and is the light transform. Let us say that `NM^-1 f = (wi', wj', wz', w)`. This gives us the coordinates of the fragment as the light “sees” it. We then compare the fragment’s depth `z'` in that space with the value stored in the shadow map at the coordinates `(i', j')`:

- If the fragment’s depth **matches** the stored depth, the fragment is visible to the light and should be lit.
- If the fragment’s depth is **greater**, it means another surface was closer to the light, and the fragment lies in shadow.

By doing this for every fragment, we obtain a binary mask of hard shadows. In this way, shadow mapping introduces a **global visibility test** into the otherwise purely local Phong shading model.

<img width="2400" height="800" alt="sum" src="https://github.com/user-attachments/assets/1bc7cc42-eb96-4d92-b48b-de09de4009ec" />


# Perspective correction
article from ssloy explanation : https://github.com/ssloy/tinyrenderer/wiki/Technical-difficulties:-linear-interpolation-with-perspective-deformations

Video explanation : https://www.youtube.com/watch?v=1Dv2-cLAJXw

When doing the shadow map, I've notice that the bottom of my image was stretch. I look a the code of ssloy and see that our rasterizer are different (Even though I copy his).

Apparently this is an issue that he did correct for his code but didnt mention it so I ask Claude and here is the problem according to him.

I've ask an IA cause i can comprehend the calculation but not the logic behind it.


Apparently it's a classic PS1-era problem called the **perspective-correct interpolation problem**.

To put it simply, screen-space position interpolates linearly, but everything else (UV, normals, color) needs to be divided by `w` first, interpolated, then divided by the interpolated `1/w` — otherwise perspective bends things that should look straight.

The more the gape between z value within the same triangle is big, the more the problem will occur.
That why we can see it a lot on the ground close to the camera.

## Core problem

When you go from 3D world space to 3D clip space, we divide the xyz by the w value from the homogenous matrix:
```cpp
vec4 ndc[3]    = { clip[0]/clip[0].w, ... };
vec2 screen[3] = { (Viewport*ndc[0]).xy(), ... };
```

But this division is not linear. It work but only for to place the vertex in the screen space, not for UV, normal or diffuse map.

Because that divide is non-linear, a straight line in UV-space does **not** map to a straight line in screen-space once the triangle is tilted in depth.

To correct this problem, we need to divide each screen-space barycentric weight by that vertex's own `w`, then renormalize so the three weights still sum to 1.
``` cpp
vec3 bc_clip = { bc_screen.x/clip[0].w, bc_screen.y/clip[1].w, bc_screen.z/clip[2].w };
bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
```


I did not understand really well what as happening in 3D in my head so those explication might be shit. 

Watch the content on top of the Lesson it's better.

# Indirect lighting (screen space ambient occlusion (ssao))

The goal is to add the a.o to the renderer to make it more realist.


When light hit a surface, it take the color of the surface and reflect to another. It's indirect lightning.

## Ambient occlusion

In phong mat, we use a constant for the ambient term. Its a crude approximation of the ambient occlusion.

To do that accuratly, we should set a reflection for each material and shoot ray from the light to know how much the indirect light change the color of the ray. But it's expensive and hard to do.

A practical approximation is ambient occlusion (AO).

To do that, we can do 1000 shadow map around a model to get as many shadow map possible. Then by averaging each point of the model in those shadow map, we can know how much this points is occluded by it's surrounding:

<img width="1280" height="719" alt="samples" src="https://github.com/user-attachments/assets/2621be84-dbeb-46a8-bd9d-51ef74c7f18e" />



## Screen-space ambient occlusion

Making many shadow map only for AO is too expensive, so we invented SSAO.

The idea is for each pixel, we take a sample of few point and compare there z-depth. If the sample point is behind then it contribute to the ambient shadowing. Then we average overt he samples to estimate how much the pixel is occluded.

<img width="800" height="800" alt="ssao" src="https://github.com/user-attachments/assets/e6c977cc-a154-43d1-9877-2754e0683ca9" />


In this example, I consider the fragment shown in red, and generate 128 samples around it. Those in blue are occluded by the surface; the ratio of green points to 128 is my estimate of indirect lighting :)


The main draw back of this technique is that it doesnt consider the model that arent render in the scene (ex: a blue screen behind the camera that make the scene a bit more blueish) .

