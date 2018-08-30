from random import random, randint
from math import sqrt

XRES, YRES = 800, 800
BOUNDARIES = (-1000, -1000), (1000, 1000)
Y_MAX_HEIGHT = 500
LIGHT_OFFSET = (0, 750, 0)
EYE_OFFSET = (-750, -250, -500)
BLOCK_SIZE_RANGE = (10, 100)
AMBIENT_LIGHT = (0.2, 0.2, 0.2)
BKRD_COLOR = (0.4325, 0.725, 0.9)
NUM_MATERIALS = 1
NUM_LIGHTS = 4
NUM_PHOTONS = 0
ROTATION = 0

CUBE_SIZE = 10
NUM_PEAKS = 20
REFLECTIVE_HEIGHT = 60
NUM_SPHERES = 40
SPHERE_HEIGHT_RANGE = (60, 750)
SPHERE_RADIUS_RANGE = (50, 100)

print '#parameters'
print XRES, YRES # resolution
print .5 # scaling factor
print 1 # antialias samples
print BOUNDARIES[1][0] * 1.25 + EYE_OFFSET[0], \
      Y_MAX_HEIGHT * 1.25 + EYE_OFFSET[1], \
      BOUNDARIES[1][1] * 1.25 + EYE_OFFSET[2] # eye
print 0, 0, 0 # focus
print 250 # focal length
print ROTATION # rotation of camera
print NUM_PHOTONS # number of photons to fire for preprocessing
print AMBIENT_LIGHT[0], AMBIENT_LIGHT[1], AMBIENT_LIGHT[2] # ambient light
print BKRD_COLOR[0], BKRD_COLOR[1], BKRD_COLOR[2] # background color 

print '#lights'
print NUM_LIGHTS + 1
for i in xrange(NUM_LIGHTS):
    print randint(BOUNDARIES[0][0], BOUNDARIES[1][0]) + LIGHT_OFFSET[0], \
          randint(0, Y_MAX_HEIGHT) + LIGHT_OFFSET[1], \
          randint(BOUNDARIES[0][1], BOUNDARIES[1][1]) + LIGHT_OFFSET[2],
    print '1 1 1 1 1 1'
    
print BOUNDARIES[1][0] * 1.25 + EYE_OFFSET[0], \
      Y_MAX_HEIGHT * 1.25 + EYE_OFFSET[1], \
      BOUNDARIES[1][1] * 1.25 + EYE_OFFSET[2], \
      '1 1 1 1 1 1'

materials = []

print '#materials'
print NUM_MATERIALS + 1
print 'reflective 0.75 0.75 0.75 0.5 0.5 0.75 0 75 0'
for i in xrange(NUM_MATERIALS):
    materials.append(str(i))
    print str(i),
    for i in xrange(3):
        print random(),
    print '0.8 0 0 0 0 0'

print '#shapes'

cubes_x, cubes_z = ((BOUNDARIES[1][0] - BOUNDARIES[0][0]) / CUBE_SIZE), ((BOUNDARIES[1][1] - BOUNDARIES[0][1]) / CUBE_SIZE)

peaks = []
for i in xrange(NUM_PEAKS):
    peaks.append(((randint(0, cubes_x - 1), randint(0, cubes_z - 1)), random() * .1 + .87))

distance = lambda a, b: sqrt((b[0] - a[0]) ** 2 + (b[1] - a[1]) ** 2)

shapes = []
for x in xrange(cubes_x):
    for z in xrange(cubes_z):
        height = 0
        for p in peaks:
            height = max(height, p[1] ** distance((x, z), p[0]))
        shapes.append('rectprism %s %f %f %f %f %f %f' % (materials[randint(0, len(materials) - 1)], \
                                                          x * CUBE_SIZE + BOUNDARIES[0][0], 0, z * CUBE_SIZE + BOUNDARIES[0][1], \
                                                          CUBE_SIZE, height * Y_MAX_HEIGHT, CUBE_SIZE))
shapes.append('rectprism reflective %f %f %f %f %f %f' % (BOUNDARIES[0][0] + 1, 0, BOUNDARIES[0][1] + 1, \
                                                          (BOUNDARIES[1][0] - BOUNDARIES[0][0]) - 2, REFLECTIVE_HEIGHT, (BOUNDARIES[1][1] - BOUNDARIES[0][1]) - 2))

for i in xrange(NUM_SPHERES):
    shapes.append('sphere reflective %f %f %f %f' % (randint(BOUNDARIES[0][0], BOUNDARIES[1][0]),
                                                     randint(SPHERE_HEIGHT_RANGE[0], SPHERE_HEIGHT_RANGE[1]),
                                                     randint(BOUNDARIES[0][1], BOUNDARIES[1][1]),
                                                     randint(SPHERE_RADIUS_RANGE[0], SPHERE_RADIUS_RANGE[1])))

print len(shapes)
for i in shapes:
    print i
