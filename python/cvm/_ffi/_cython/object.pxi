# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
"""Maps object type to its constructor"""
cdef list OBJECT_TYPE = []


def _register_object(int index, object cls):
    """register object class"""
    if issubclass(cls, NDArrayBase):
        _register_ndarray(index, cls)
        return

    global OBJECT_TYPE
    while len(OBJECT_TYPE) <= index:
        OBJECT_TYPE.append(None)
    OBJECT_TYPE[index] = cls


cdef class ObjectBase:
    cdef void* chandle

    cdef inline _set_handle(self, handle):
        cdef unsigned long long ptr
        if handle is None:
            self.chandle = NULL
        else:
            ptr = handle.value
            self.chandle = <void*>(ptr)

    property handle:
        def __get__(self):
            return ctypes_handle(self.chandle)

        def __set__(self, value):
            self._set_handle(value)

    def __dealloc__(self):
        CALL(CVMObjectFree(self.chandle))

    def __init_handle_by_constructor__(self, fconstructor, *args):
        """Initialize the handle by calling constructor function

        Parameters
        ----------
        fconstructor : Function
            Constructor function.

        args : list of objects
            The arguments to the constructor

        Returns
        -------

        """
        self.chandle = NULL
        # cdef void* chandle
        # ConstructorCall(
        #     (<PackedFuncBase>fconstructor).chandle,
        #     kCVMObjectHandle, args, &chandle)
        # self.chandle = chandle

    def same_as(self, other):
        """Check object identity.

        Parameters
        ----------
        other

        Returns
        -------

        """
        if not isinstance(other, ObjectBase):
            return False
        return self.chandle == (<ObjectBase>other).chandle
