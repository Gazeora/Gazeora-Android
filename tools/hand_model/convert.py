"""Makes app/src/main/cpp/hands/hand_model_data.h from the "VR Hand" model's glTF.

The model: "VR Hand" by RockDutyFree_models,
https://sketchfab.com/3d-models/vr-hand-a647cce524254c9a96d4244c12811d7a, under CC BY 4.0.
Download it as glTF, then:

    python tools/hand_model/convert.py path/to/scene.gltf

It takes the right hand's shape: its skin in its rest pose, each corner's bones mapped onto the
hand skeleton's (hands/hand_skeleton.h), how much of each corner shows (all of it, but fading
out toward the wrist), and the rest pose's joints in MediaPipe's landmark order, all in meters
on the skeleton's palm. Not its texture: the hands are drawn in one color of their own
(shaders/hand.frag). It prints the skeleton's measures, which hand_skeleton.h takes from it.
Needs numpy.
"""

import json
import os
import struct
import sys

import numpy as np

# The skeleton's palm, from the wrist to the middle finger's knuckle, in meters.
PALM_LENGTH = 0.09
# The model's joints, by name, in MediaPipe's landmark order: the wrist, then each finger from
# its base out, the thumb first.
LANDMARK_JOINTS = [
    'HANDPALM',
    'THUMB_BASE', 'THUMB_MID', 'THUMB_TOP', 'THUMB_UP_TOP',
    'INDEX_BASE', 'INDEX_MID', 'INDEX_TOP', 'INDEX_UP_TOP',
    'MIDDLE_F_BASE', 'MIDDLE_F_MID', 'MIDDLE_F_TOP', 'MIDDLE_F_UP_TOP',
    'RING_BASE', 'RING_MID', 'RING_TOP', 'RING_UP_TOP',
    'PINK_BASE', 'PINK_MID', 'PINK_TOP', 'PINK_UP_TOP',
]
# The skin ends at the wrist; it fades out before, over this far along it, in meters from the
# wrist.
FADES_FROM = -0.005
FADES_TO = 0.035
FINGERS = 5
BONES = 3
OUT = os.path.join(os.path.dirname(__file__), '..', '..', 'app', 'src', 'main', 'cpp', 'hands',
                   'hand_model_data.h')

COMPONENT = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
WIDTH = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def load(path):
    gltf = json.load(open(path, encoding='utf-8'))
    data = open(os.path.join(os.path.dirname(path), gltf['buffers'][0]['uri']), 'rb').read()

    def accessor(i):
        a = gltf['accessors'][i]
        view = gltf['bufferViews'][a['bufferView']]
        n, kind = WIDTH[a['type']], COMPONENT[a['componentType']]
        size = struct.calcsize(kind)
        stride = view.get('byteStride', size * n)
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        rows = [struct.unpack_from('<' + kind * n, data, start + k * stride)
                for k in range(a['count'])]
        return np.array(rows, dtype=float)
    return gltf, accessor


def joint_name(name):
    """The joint's name without the exporter's numbering: INDEX_BASE_joint_03 -> INDEX_BASE."""
    return name.split('_joint')[0]


def our_bone(name):
    """The skeleton's bone (RiggedHand's numbering) a model joint moves with."""
    if name in LANDMARK_JOINTS[1:]:
        landmark = LANDMARK_JOINTS.index(name)
        finger, joint = (landmark - 1) // 4, (landmark - 1) % 4
        return 1 + finger * BONES + min(joint, BONES - 1)
    return 0  # the palm, the wrist and the forearm


def palm_axes(at):
    """The palm's along, across and palmward, as the skeleton's fit finds them: along to the
    middle knuckle, across along the knuckles' line, square to it."""
    wrist, index, middle, little = at[0], at[5], at[9], at[17]
    along = (middle - wrist) / np.linalg.norm(middle - wrist)
    line = index - little
    across = line - along * line.dot(along)
    across /= np.linalg.norm(across)
    return along, across, np.cross(across, along)  # palmward, for a right hand


def number(v):
    """`v` as a C++ float literal, to five places: 0.25f, 1.0f."""
    text = ('%.5f' % v).rstrip('0')
    if text.endswith('.'):
        text += '0'
    return ('0.0' if text == '-0.0' else text) + 'f'


def main(path):
    gltf, accessor = load(path)
    skin = gltf['skins'][0]
    names = [joint_name(gltf['nodes'][j]['name']) for j in skin['joints']]
    rest = np.linalg.inv(accessor(skin['inverseBindMatrices']).reshape(-1, 4, 4)
                         .transpose(0, 2, 1))
    joints = np.array([rest[names.index(n)][:3, 3] for n in LANDMARK_JOINTS])

    primitive = gltf['meshes'][0]['primitives'][0]
    attributes = primitive['attributes']
    positions = accessor(attributes['POSITION'])
    normals = accessor(attributes['NORMAL'])
    bones = accessor(attributes['JOINTS_0']).astype(int)
    weights = accessor(attributes['WEIGHTS_0'])
    triangles = accessor(primitive['indices']).astype(int).ravel()

    # Onto the skeleton's palm, in meters: x along, y across, z back (away from the palm).
    along, across, palmward = palm_axes(joints)
    scale = PALM_LENGTH / np.linalg.norm(joints[9] - joints[0])
    turn = np.array([along, across, -palmward])

    def place(p):
        return (p - joints[0]) @ turn.T * scale
    joints, positions = place(joints), place(positions)
    normals = normals @ turn.T
    normals /= np.linalg.norm(normals, axis=1, keepdims=True)

    fade = np.clip((positions[:, 0] - FADES_FROM) / (FADES_TO - FADES_FROM), 0, 1)
    shows = fade * fade * (3 - 2 * fade)

    # Each corner's bones, the skeleton's, with their weights adding up to 1.
    mapped = []
    for corner_bones, corner_weights in zip(bones, weights):
        merged = {}
        for b, w in zip(corner_bones, corner_weights):
            if w > 0:
                ours = our_bone(names[b])
                merged[ours] = merged.get(ours, 0) + w
        strongest = sorted(merged.items(), key=lambda item: -item[1])[:4]
        total = sum(w for _, w in strongest)
        strongest += [(0, 0.0)] * (4 - len(strongest))
        mapped.append([(b, w / total) for b, w in strongest])

    write(joints, positions, normals, shows, mapped, triangles)
    report(joints)


def write(joints, positions, normals, shows, mapped, triangles):
    lines = [
        '// Generated by tools/hand_model/convert.py: do not edit.',
        '// From "VR Hand" by RockDutyFree_models',
        '// (https://sketchfab.com/3d-models/vr-hand-a647cce524254c9a96d4244c12811d7a),',
        '// licensed under CC BY 4.0 (http://creativecommons.org/licenses/by/4.0/): its right',
        '// hand\'s shape, posed on the hand skeleton\'s palm.',
        '#pragma once',
        '',
        '#include <cstdint>',
        '',
        'namespace gazeora {',
        '',
        'namespace hand_model {',
        '',
        '// A corner of the skin at rest, in meters on the palm: x along it from the wrist, y',
        '// across toward the thumb\'s side, z back from it; its normal, the same way; how much of it',
        '// shows, fading out toward the wrist; and the skeleton\'s bones it moves with',
        '// (RiggedHand\'s numbering), with how much.',
        'struct Corner {',
        '    float x, y, z;',
        '    float nx, ny, nz;',
        '    float shows;',
        '    uint8_t bones[4];',
        '    float weights[4];',
        '};',
        '',
        '// The rest pose\'s joints, in MediaPipe\'s landmark order, the same way.',
        'constexpr float kJoints[21][3] = {',
    ]
    for j in joints:
        lines.append('    {%s},' % ', '.join(number(v) for v in j))
    lines += ['};', '', 'constexpr Corner kCorners[] = {']
    for p, n, a, m in zip(positions, normals, shows, mapped):
        lines.append('    {%s, %s, %s, {%s}, {%s}},' % (
            ', '.join(number(v) for v in p), ', '.join(number(v) for v in n),
            number(a), ', '.join(str(b) for b, _ in m),
            ', '.join(number(w) for _, w in m)))
    lines += ['};', '', 'constexpr uint16_t kTriangles[] = {']
    for k in range(0, len(triangles), 12):
        lines.append('    ' + ', '.join(str(t) for t in triangles[k:k + 12]) + ',')
    lines += ['};', '', '}  // namespace hand_model', '', '}  // namespace gazeora', '']
    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))


def report(joints):
    """The skeleton's measures (hand_skeleton.h) from the rest pose: each finger's base along,
    across and palmward of the palm, and its bones' lengths."""
    print('kKnuckles (along, across, palmward):')
    for f in range(FINGERS):
        base = joints[1 + f * 4]
        print('    {%.3ff, %.3ff, %.3ff},' % (base[0], base[1], -base[2]))
    print('kLengths:')
    for f in range(FINGERS):
        chain = joints[1 + f * 4:5 + f * 4]
        print('    {%s},' % ', '.join('%.3ff' % np.linalg.norm(chain[b + 1] - chain[b])
                                      for b in range(BONES)))
    thumb = joints[2] - joints[1]
    thumb /= np.linalg.norm(thumb)
    print('thumb rest (along, across, palmward): %.2f, %.2f, %.2f' % (
        thumb[0], thumb[1], -thumb[2]))


if __name__ == '__main__':
    main(sys.argv[1])
