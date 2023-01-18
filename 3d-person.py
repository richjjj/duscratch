import numpy as np
import cv2


cos_angle = lambda x: np.math.cos(x / 180 * np.pi)
sin_angle = lambda x: np.math.sin(x / 180 * np.pi)
tan_angle = lambda x: np.math.tan(x / 180 * np.pi)


def rodrigues_rotation(n, angle):
    # n旋转轴[3x1]
    # angle为旋转角度
    # 假定旋转是过原点的，起点是原点，n是旋转轴
    
    nx, ny, nz = n[0, 0], n[1, 0], n[2, 0]
    M = np.array([
        [0, -nz, ny],
        [nz, 0, -nx],
        [-ny, nx, 0]
    ])
    R = np.eye(4)
    R[:3, :3] = cos_angle(angle) * np.eye(3) + (1 - cos_angle(angle)) * n @ n.T + sin_angle(angle) * M
    return R

def get_view_matrix(e, g, t):
    
    T_view = np.array([
        [1, 0, 0, -e[0, 0]],
        [0, 1, 0, -e[1, 0]],
        [0, 0, 1, -e[2, 0]],
        [0, 0, 0, 1],
    ])
    
    g_cross_t = np.cross(g.T, t.T).T
    R_view = np.array([
        [g_cross_t[0, 0], t[0, 0], -g[0, 0], 0],
        [g_cross_t[1, 0], t[1, 0], -g[1, 0], 0],
        [g_cross_t[2, 0], t[2, 0], -g[2, 0], 0],
        [0, 0, 0, 1]
    ]).T
    return R_view @ T_view
    #return T_view

def get_perspective_matrix(eye_fov, aspect_ratio, near, far):
    
    t = -abs(near) * tan_angle(eye_fov / 2.0)
    r = t * aspect_ratio
    
    return np.array([
        [near / r, 0, 0, 0],
        [0, near / r, 0, 0],
        [0, 0, (near + far) / (near - far), -2 * near * far / (near - far)],
        [0, 0, 1, 0]
    ])

def get_viewport_matrix(width, height, near, far):
    return np.array([
        [width / 2, 0, 0, width / 2],
        [0, -height / 2, 0, height / 2],
        [0, 0, -(near - far) / 2.0, (near + far) / 2.0],
        [0, 0, 0, 1],
    ])

def rotation(ax, ay, az):
    return rodrigues_rotation(np.array([[0, 0, 1]]).T, az) @ rodrigues_rotation(np.array([[1, 0, 0]]).T, ax) @ rodrigues_rotation(np.array([[0, 1, 0]]).T, ay)

def get_ground(w=3, h=3):

    w /= 2
    h /= 2
    return np.array([
        [-w, 0, -h],
        [+w, 0, -h],
        [+w, 0, +h],
        [-w, 0, +h]
    ])[[0, 1, 1, 2, 2, 3, 3, 0]]


def get_mesh(w=3, h=3, n=10, ori="h"):
    
    if ori == "h":
        oy = -h / 2
        ys = np.linspace(0, h, n) + oy

        x0 = np.full(n, -w/2)
        x1 = np.full(n, +w/2)
        p  = np.zeros((2*n, 3))
        p[::2, 0]  = x0
        p[1::2, 0] = x1
        p[::2, 2]  = ys
        p[1::2, 2] = ys
        return p
    elif ori == "w":
        ox = -w / 2
        xs = np.linspace(0, w, n) + ox

        y0 = np.full(n, -h/2)
        y1 = np.full(n, +h/2)
        p  = np.zeros((2*n, 3))
        p[::2, 2]  = y0
        p[1::2, 2] = y1
        p[::2, 0]  = xs
        p[1::2, 0] = xs
        return p


def get_mesh_box(w=3, h=3, n=10):

    x, z = np.meshgrid(np.linspace(0, w, n), np.linspace(0, h, n))
    xz = np.stack([x - w/2, np.zeros_like(x), z - h/2], axis=-1)
    return xz


def draw_lines(image, vmvp, points, color=(0, 255, 0), width=1):

    h, w   = image.shape[:2]
    n      = len(points)
    points = np.concatenate([points, np.ones((n, 1))], axis=-1)
    proj   = vmvp @ points.T
    points = (proj / proj[3]).T

    for p0, p1 in zip(points[::2], points[1::2]):

        x1, y1, z1, w1 = p0
        x2, y2, z2, w2 = p1
        if z1 >= near or z2 >= near or abs(x1) > 10000 or abs(y1) > 10000 or abs(x2) > 10000 or abs(y2) > 10000:
            continue

        p0 = tuple(map(int, [x1 * 16, y1 * 16]))
        p1 = tuple(map(int, [x2 * 16, y2 * 16]))
        cv2.line(image, p0, p1, color, width, 16, 4)


def draw_meshbox(image, vmvp, points, color=(0, 255, 0), width=1):

    h, w   = image.shape[:2]
    ph, pw = points.shape[:2]
    n      = ph * pw
    points = np.concatenate([points.reshape(n, 3), np.ones((n, 1))], axis=-1)
    proj   = vmvp @ points.T
    points = (proj / proj[3]).T.reshape(ph, pw, 4)

    for iw in range(pw):
        for ih in range(ph-1):
            x1, y1, z1, w1 = points[ih, iw]
            x2, y2, z2, w2 = points[ih+1, iw]
            if z1 >= near or z2 >= near or abs(x1) > 10000 or abs(y1) > 10000 or abs(x2) > 10000 or abs(y2) > 10000:
                continue

            p0 = tuple(map(int, [x1*16, y1*16]))
            p1 = tuple(map(int, [x2*16, y2*16]))
            cv2.line(image, p0, p1, color, width, 16, 4)

    for ih in range(ph):
        for iw in range(pw-1):
            x1, y1, z1, w1 = points[ih, iw]
            x2, y2, z2, w2 = points[ih, iw+1]
            if z1 >= near or z2 >= near or abs(x1) > 10000 or abs(y1) > 10000 or abs(x2) > 10000 or abs(y2) > 10000:
                continue

            p0 = tuple(map(int, [x1*16, y1*16]))
            p1 = tuple(map(int, [x2*16, y2*16]))
            cv2.line(image, p0, p1, color, width, 16, 4)


def draw_meshbox2(image, vmvp, points, color=(0, 255, 0), width=-1):

    h, w   = image.shape[:2]
    ph, pw = points.shape[:2]
    n      = ph * pw
    points = np.concatenate([points.reshape(n, 3), np.ones((n, 1))], axis=-1)
    proj   = vmvp @ points.T
    points = (proj / proj[3]).T.reshape(ph, pw, 4)

    for iw in range(pw-1):
        for ih in range(ph-1):
            x1, y1, z1, w1 = points[ih  , iw  ]
            x2, y2, z2, w2 = points[ih  , iw+1]
            x3, y3, z3, w3 = points[ih+1, iw+1]
            x4, y4, z4, w4 = points[ih+1, iw  ]
            if z1 >= near or z2 >= near or abs(x1) > 10000 or abs(y1) > 10000 or abs(x2) > 10000 or abs(y2) > 10000:
                continue

            if z3 >= near or z4 >= near or abs(x3) > 10000 or abs(y3) > 10000 or abs(x4) > 10000 or abs(y4) > 10000:
                continue

            ps = np.array([x1, y1, x2, y2, x3, y3, x4, y4], dtype=np.int32).reshape(1, -1, 2)
            cv2.fillPoly(image, ps, color)


def get_people(s=0.5):

    # return np.array([
    #     0, s, 0,
    #     -s, 0, 0,
    #     +s, 0, 0,
    #     0, 3*s, 0,
    #     -s, 2*s, 0,
    #     +s, 2*s, 0,
    #     -s/3, 3*s, 0,
    #     0, 2.8*s, 0,
    #     +s/3, 3*s, 0
    # ]).reshape(-1, 3)[[0, 1, 0, 2, 0, 3, 4, 5, 6, 7, 7, 8]]
    return np.array([
        -s/2, 0, -s/2,
        +s/2, 0, -s/2,
        +s/2, 0, +s/2,
        -s/2, 0, +s/2,
        -s/2, s, -s/2,
        +s/2, s, -s/2,
        +s/2, s, +s/2,
        -s/2, s, +s/2,
    ]).reshape(-1, 3)[[0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7, 0, 6, 1, 7, 2, 4, 3, 5]]


def translate(tx=0, ty=0, tz=0):

    return np.array([
        [1, 0, 0, tx],
        [0, 1, 0, ty],
        [0, 0, 1, tz],
        [0, 0, 0, 1]
    ])

px, py, pz = 0, 0, 5
#e = np.array([[px, py + 1, pz]], dtype=np.float32).T
#g = np.array([[px, py, pz + -1]], dtype=np.float32).T
#t = np.array([[px, py + 1,  pz]], dtype=np.float32).T
people       = translate(px, py, pz)
mesh_box     = get_mesh_box(13, 15, n=20)
view_matrix  = translate(-px, -py - 1, -pz - 2) #get_view_matrix(e, g, t)

trans_speed  = 0.05
rotate_speed = 3
sense_width  = 600
sense_height = 600
fov  = 45
near = 0.1
far  = 50
ground  = get_ground(3, 5)
pground = get_ground(3, 5)
image   = np.full((sense_height, sense_width, 3), 0, dtype=np.uint8)
topimage = np.full((sense_height, sense_width, 3), 0, dtype=np.uint8)

axes  = np.array([
    [0, 0, 0],
    [1, 0, 0],
    [0, 1, 0],
    [0, 0, 1],
])[[0, 1, 0, 2, 0, 3]]

pp = get_people()
e = np.array([[0, 20, 0]], dtype=np.float32).T
g = np.array([[0, -1, 0]], dtype=np.float32).T
t = np.array([[0, 0,  -1]], dtype=np.float32).T
topview = get_view_matrix(e, g, t)

proj_matrix      = get_perspective_matrix(fov, 1, near, far)
view_port_matrix = get_viewport_matrix(sense_width, sense_height, near, far)
top_mvp          = view_port_matrix @ proj_matrix @ topview

while True:
    vmvp             = view_port_matrix @ proj_matrix @ rotation(15, 0, 0) @ view_matrix
    
    topimage[:] = 0
    image[:] = 0
    draw_meshbox2(image, vmvp, mesh_box, (128, 128, 128), 1)
    draw_meshbox(image, vmvp, mesh_box, (255, 255, 255), 1)
    draw_lines(image, vmvp, ground, (0, 0, 255), 2)
    draw_lines(image, vmvp, axes, (0, 255, 255), 2)
    draw_lines(image, vmvp @ people, pp, (255, 128, 0), 2)

    draw_meshbox2(topimage, top_mvp, mesh_box, (128, 128, 128), 1)
    draw_meshbox(topimage, top_mvp, mesh_box, (255, 255, 255), 1)
    draw_lines(topimage, top_mvp, ground, (0, 0, 255), 2)
    draw_lines(topimage, top_mvp, axes, (0, 255, 255), 2)
    draw_lines(topimage, top_mvp @ people, pp, (255, 128, 0), 2)
    
    cv2.imshow("image", image)
    cv2.imshow("top", topimage)
    key = cv2.waitKey(1) & 0xFF

    if key == ord("w"):
        view_matrix  = translate(tz=+trans_speed) @ view_matrix
        people       = people @ translate(tz=-trans_speed)
    elif key == ord("s"):
        view_matrix  = translate(tz=-trans_speed) @ view_matrix
        people       = people @ translate(tz=+trans_speed)
    elif key == ord("a"):
        view_matrix  = translate(tx=+trans_speed) @ view_matrix
        people       = people @ translate(tx=-trans_speed)
    elif key == ord("d"):
        view_matrix  = translate(tx=-trans_speed) @ view_matrix
        people       = people @ translate(tx=+trans_speed)
    elif key == ord("u"):
        invt = np.linalg.inv(people)
        view_matrix  = view_matrix @ people @ rotation(0,  rotate_speed, 0) @ invt
        people       = people @ rotation(0, -rotate_speed, 0)
    elif key == ord("j"):
        invt = np.linalg.inv(people)
        view_matrix  = view_matrix @ people @ rotation(0,  -rotate_speed, 0) @ invt
        people       = people @ rotation(0, +rotate_speed, 0)
    elif key == ord("z"):
        break

