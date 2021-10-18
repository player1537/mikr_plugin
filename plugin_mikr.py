#!/usr/bin/env python
"""

"""

from __future__ import annotations
from collections import namedtuple
from contextlib import redirect_stderr
import csv
from dataclasses import dataclass
from io import TextIOWrapper
from itertools import permutations
from math import copysign
import os
from pathlib import Path
import struct
import sys
from typing import NewType
from zipfile import ZipFile as zip_open

import numpy as np
if sys.version_info >= (3, 10, 0):
    from zipfile import Path as ZipPath
else:
    # Python before 3.10 has a bug with zipfile.Path, but Python library zipp
    # has a fix. https://bugs.python.org/issue40564
    from zipp import Path as ZipPath


PointID = NewType('PointID', str)
BoxID = NewType('BoxID', str)
StressID = NewType('StressID', BoxID)
Timestep = NewType('Timestep', str)
class NoValidBox(Exception):
    pass

VertexPositionArray = NewType('VertexPositionArray', np.ndarray)
IndexArray = NewType('IndexArray', np.ndarray)
CellIndexArray = NewType('CellIndexArray', np.ndarray)
CellDataArray = NewType('CellDataArray', np.ndarray)


@dataclass
class Mikr:
    root: Union[Path, ZipPath]
    points: Dict[PointID, Point]
    boxes: Dict[BoxID, Box]
    timesteps: List[Timestep]
    timestep: Optional[Timestep]
    stresses: Optional[Dict[BoxID, Stress]]

    @classmethod
    def parseall(
        cls,
        root: Union[Path, ZipPath],
    ) -> Mikr:
        with (root / 'nodes.csv').open('r') as f:
            #f = TextIOWrapper(f, encoding='utf-8')
            points = Point.parseall(f)
        
        with (root / 'elements.csv').open('r') as f:
            #f = TextIOWrapper(f, encoding='utf-8')
            boxes = Box.parseall(f, points)

        timesteps = []
        for path in (root / 'S').iterdir():
            assert path.is_file(), '{path=}'
            name = path.name
            stem, suffix = name.split('.', 1)
            assert suffix == 'csv'
            timesteps.append(stem)
        timesteps.sort(key=int)

        timestep = None
        stresses = None
        
        return cls(root, points, boxes, timesteps, timestep, stresses)

    def load(
        self,
        timestep: Timestep,
    ):
        assert timestep in self.timesteps
        with (self.root / 'S' / f'{timestep}.csv').open('r') as f:
            stresses = Stress.parseall(f)
        self.stresses = stresses
        self.timestep = timestep
    
    def as_numpy(
        self,
    ) -> Tuple[VertexPositionArray, IndexArray, CellIndexArray, CellDataArray]:
        assert self.timestep is not None
        NP = len(self.points)
        plookup: Dict[PointID, int] = {}
        vertex_position = np.ones((NP, 3), dtype='float32')
        vertex_position[:] *= -373737
        for pid, point in self.points.items():
            plookup.setdefault(pid, len(plookup))
            vertex_position[plookup[pid], :] = point.x, point.y, point.z
        assert -373737 not in vertex_position

        NB = len(self.boxes)
        blookup: Dict[BoxID, int] = {}
        index = np.ones((NB, 8), dtype='int32')
        index[:] *= -373737
        for bid, box in self.boxes.items():
            blookup.setdefault(bid, len(blookup))
            index[blookup[bid], :] = (
                # four bottom vertices counterclockwise
                plookup[box.x0y0z0],
                plookup[box.x1y0z0],
                plookup[box.x1y1z0],
                plookup[box.x0y1z0],
                # four top vertices counterclockwise
                plookup[box.x0y0z1],
                plookup[box.x1y0z1],
                plookup[box.x1y1z1],
                plookup[box.x0y1z1],
            )
        assert -373737 not in index

        cell_index = np.arange(0, NB, dtype='uint32').reshape((NB, 1))
        cell_index[:] *= 8

        cell_type = np.ones((NB, 1), dtype='uint8')
        cell_type[:] *= 12

        cell_data = np.ones((NB, 1), dtype='float32')
        cell_data[:] *= -373737
        for bid, stress in self.stresses.items():
            if (i := blookup.get(bid, None)) is None:
                print(f'{bid=} not in blookup', file=sys.stderr)
                continue
            cell_data[i, :] = stress.s11
        it = iter(zip(range(0, NB-1), range(1, NB)))
        for i, _ in it:
            if cell_data[i] == -373737:
                break
        cutoff = i
        for i, j in it:
            assert cell_data[i] == cell_data[j], f'{i} {j} {cell_data[i]=} {cell_data[j]=}'
        
        cutoff = 10
        
        print(f'Cutting off {NB-cutoff} elements', file=sys.stderr)
        index = index[:cutoff, :]
        cell_index = cell_index[:cutoff, :]
        cell_type = cell_type[:cutoff, :]
        cell_data = cell_data[:cutoff, :]
            
        assert -373737 not in vertex_position, f'{np.where(vertex_position == -373737)}'
        assert -373737 not in index, f'{np.where(index == -373737)}'
        assert -373737 not in cell_index, f'{np.where(cell_index == -373737)}'
        assert -373737 not in cell_type, f'{np.where(cell_type == -373737)}'
        assert -373737 not in cell_data, f'{np.where(cell_data == -373737)}'

        print(f'{index=}', file=sys.stderr)

        return vertex_position, index, cell_index, cell_type, cell_data


@dataclass
class Point:
    x: float
    y: float
    z: float

    @classmethod
    def parseall(
        cls,
        f: TextIO,
    ) -> Dict[PointID, Point]:
        points = {}
        reader = csv.reader(f)
        for row in reader:
            pid = PointID(row[0])
            x = float(row[1])
            y = float(row[2])
            z = float(row[3])

            points[pid] = cls(x, y, z)
        return points


@dataclass
class Box:
    x0y0z0: PointID
    x0y0z1: PointID
    x0y1z0: PointID
    x0y1z1: PointID
    x1y0z0: PointID
    x1y0z1: PointID
    x1y1z0: PointID
    x1y1z1: PointID

    @property
    def pids(
        self,
    ) -> List[PointID]:
        return (
            self.x0y0z0,
            self.x0y0z1,
            self.x0y1z0,
            self.x0y1z1,
            self.x1y0z0,
            self.x1y0z1,
            self.x1y1z0,
            self.x1y1z1,
        )
    
    @pids.setter
    def pids(
        self,
        pids,
    ) -> None:
        (
            self.x0y0z0,
            self.x0y0z1,
            self.x0y1z0,
            self.x0y1z1,
            self.x1y0z0,
            self.x1y0z1,
            self.x1y1z0,
            self.x1y1z1,
        ) = pids

    @classmethod
    def parseall(
        cls,
        f: TextIO,
        points: Dict[PointID, Point],
    ) -> Dict[BoxID, Box]:
        boxes = {}
        reader = csv.reader(f)
        for i, row in enumerate(reader):
            corners = []
            bid = BoxID(row[0])
            pids = [PointID(x) for x in row[1:]]

            self = cls(*pids)
            if not self._reorder(points):
                print(f'Bad box {i=} {self=}', file=sys.stderr)
                continue
            boxes[bid] = self

        return boxes
    
    def _reorder(
        self,
        points: Dict[PointID, Point],
    ):
        if self._validate(points):
            return True
        elif self._reorder_relative_to_center(points):
            return True
        elif self._reorder_all_permutations(points):
            return True
        else:
            return False

    def _reorder_relative_to_center(
        self,
        points: Dict[PointID, Point],
    ) -> None:
        order = [
            (-1, -1, -1),
            (-1, -1, +1),
            (-1, +1, -1),
            (-1, +1, +1),
            (+1, -1, -1),
            (+1, -1, +1),
            (+1, +1, -1),
            (+1, +1, +1),
        ]
        center = Point(
            (sum(points[pid].x for pid in self.pids) / len(self.pids)),
            (sum(points[pid].y for pid in self.pids) / len(self.pids)),
            (sum(points[pid].z for pid in self.pids) / len(self.pids)),
        )
        self.pids = sorted(self.pids, key=lambda pid: order.index((
            copysign(1, points[pid].x - center.x),
            copysign(1, points[pid].y - center.y),
            copysign(1, points[pid].z - center.z),
        )))
        return self._validate(points)
    
    def _reorder_all_permutations(
        self,
        points: Dict[PointID, Point],
    ) -> List[PointID]:
        for self.pids in permutations(self.pids):
            if self._validate(points):
                return True
        else:
            return False

    def _validate(
        self,
        points: Dict[PointID, Point],
    ) -> bool:
        try:
            assert points[self.x0y0z0].x <= points[self.x1y0z0].x, f'{self=}'
            assert points[self.x0y0z1].x <= points[self.x1y0z1].x, f'{self=}'
            assert points[self.x0y1z0].x <= points[self.x1y1z0].x, f'{self=}'
            assert points[self.x0y1z1].x <= points[self.x1y1z1].x, f'{self=}'

            assert points[self.x0y0z0].y <= points[self.x0y1z0].y, f'{self=}'
            assert points[self.x0y0z1].y <= points[self.x0y1z1].y, f'{self=}'
            assert points[self.x1y0z0].y <= points[self.x1y1z0].y, f'{self=}'
            assert points[self.x1y0z1].y <= points[self.x1y1z1].y, f'{self=}'

            assert points[self.x0y0z0].z <= points[self.x0y0z1].z, f'{self=}'
            assert points[self.x0y1z0].z <= points[self.x0y1z1].z, f'{self=}'
            assert points[self.x1y0z0].z <= points[self.x1y0z1].z, f'{self=}'
            assert points[self.x1y1z0].z <= points[self.x1y1z1].z, f'{self=}'
        except AssertionError:
            return False
        else:
            return True


@dataclass
class Stress:
    s11: float
    s22: float
    s33: float
    s12: float
    s13: float
    s23: float

    @classmethod
    def parseall(
        cls,
        f: TextIO,
    ) -> Dict[BoxID, Stress]:
        stresses = {}
        reader = csv.reader(f)
        next(reader)  # skip header
        for row in reader:
            bid = BoxID(row[0])
            s11 = float(row[1])
            s22 = float(row[2])
            s33 = float(row[3])
            s12 = float(row[4])
            s13 = float(row[5])
            s23 = float(row[6])

            stresses[bid] = cls(s11, s22, s33, s12, s13, s23)
        
        return stresses


def main(root):
    with os.fdopen(sys.stdout.fileno(), 'wb', closefd=False) as stdout, \
         os.fdopen(sys.stdin.fileno(), 'rb', closefd=False) as stdin:
        print(f'Hello from {__file__}', file=sys.stderr)

        def write(fmt, *args):
            print(f'write({fmt=})', file=sys.stderr)
            stdout.write(struct.pack(fmt, *args))
        def read(fmt):
            print(f'read({fmt=})', file=sys.stderr)
            return struct.unpack(fmt, stdin.read(struct.calcsize(fmt)))

        # Start
        n, = read('@n')
        if n < 0:
            return
        
        # Load
        mikr = Mikr.parseall(root)
        write('@n', len(mikr.timesteps))
        for timestep in mikr.timesteps:
            timestep = timestep.encode('utf-8')
            write('@n', len(timestep))
            write(f'{len(timestep)}s', timestep)
        stdout.flush()

        while True:
            # Transfer
            timestep_index, = read('@n')
            if timestep_index < 0:
                break
            mikr.load(mikr.timesteps[timestep_index])
            vertex_position, index, cell_index, cell_type, cell_data = mikr.as_numpy()
            NP = vertex_position.shape[0]
            NB = index.shape[0]
            write('@n', NP)
            write('@n', NB)
            for arr in (vertex_position, index, cell_index, cell_type, cell_data):
                print(f'{arr.dtype=}', file=sys.stderr)
                write('@n', arr.nbytes)
                write(f'{arr.nbytes}s', arr.tobytes())
            stdout.flush()


def cli():
    def zipfile(s):
        path = Path(s)
        parts = path.parts
        root = Path(parts[0])
        for part in parts[1:]:
            path = root / part
            if path.is_dir():
                root = path
                continue
            assert path.is_file() and path.suffix == '.zip'
            root = ZipPath(zip_open(path, 'r'))
        return root

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--zip',
        type=Path,
        default=None,
        dest='root',
    )
    args = vars(parser.parse_args())

    if args['root'] is None:
        args['root'] = zipfile(Path.cwd() / 'data' / 'bridge_15mm' / 'bridge_15mm_subset.zip' / 'bridge_15mm')

    main(**args)


if __name__ == '__main__':
    cli()