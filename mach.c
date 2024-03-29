/*
 *  mach.c
 *  PyMach
 *
 *  Pedro José Pereira Vieito © 2016
 *  Based in Andrew Barnert (abarnert) pymach:
 *  https://github.com/abarnert/pymach
 *
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <sys/types.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <mach/mach_types.h>
#include <mach/mach_vm.h>
#include "attach.h"

static PyObject *MachError;

static PyObject *PyMach_Error(kern_return_t ret) {
    return PyErr_Format(MachError, "%s", mach_error_string(ret));
}

static PyObject *pymach_task_self(PyObject *self, PyObject *args) {
    return Py_BuildValue("i", mach_task_self());
}

static PyObject *pymach_task_for_pid(PyObject *self, PyObject *args) {
    int pid;
    task_t task;
    kern_return_t ret;
    
    if (!PyArg_ParseTuple(args, "i", &pid))
        return NULL;
        
    ret = task_for_pid(mach_task_self(), pid, &task);
    if (ret)
        return PyMach_Error(ret);
        
    return Py_BuildValue("i", task);
}

static PyObject *pymach_vm_protect(PyObject *self, PyObject *args) {
    task_t task;
    mach_vm_address_t address;
    mach_vm_size_t size;
    int prot;
    kern_return_t ret;
    
    if (!PyArg_ParseTuple(args, "iKLi", &task, &address, &size, &prot))
        return NULL;
        
    ret = mach_vm_protect(task, address, size, 0, prot);
    if (ret)
        return PyMach_Error(ret);
        
    Py_RETURN_NONE;
}

static PyObject *pymach_vm_read(PyObject *self, PyObject *args) {
    task_t task;
    mach_vm_address_t address;
    mach_vm_size_t size;
    vm_offset_t data;
    mach_msg_type_number_t dataCnt;
    kern_return_t ret;
    
    if (!PyArg_ParseTuple(args, "iKL", &task, &address, &size))
        return NULL;
        
    ret = mach_vm_read(task, address, size, &data, &dataCnt);
    if (ret)
        return PyMach_Error(ret);
        
    PyObject *val = Py_BuildValue("y#", data, dataCnt);
    mach_vm_deallocate(mach_task_self(), data, dataCnt);
    return val;
}

static PyObject *pymach_vm_write(PyObject *self, PyObject *args) {
    task_t task;
    mach_vm_address_t address;
    char *buf;
    int len;
    kern_return_t ret;
    
    if (!PyArg_ParseTuple(args, "iKy#", &task, &address, &buf, &len))
        return NULL;
        
    ret = mach_vm_write(task, address, (vm_offset_t)buf, len);
    if (ret)
        return PyMach_Error(ret);
        
    Py_RETURN_NONE;
}

static PyObject *pymach_vm_asrl_offset(PyObject *self, PyObject *args) {
    pid_t pid;
    mach_vm_address_t address;
    uint64_t asrl_offset;
    kern_return_t ret;
    
    if (!PyArg_ParseTuple(args, "i", &pid))
        return NULL;
    
    ret = find_main_binary(pid, &address);
    if (ret)
        return PyMach_Error(ret);
    
    ret = get_image_size(address, pid, &asrl_offset);
    if (ret == -1)
        return PyMach_Error(ret);
    
    PyObject *val = Py_BuildValue("i", asrl_offset);
    return val;
}

static PyMethodDef mach_methods[] = {
    {"task_self", pymach_task_self, METH_VARARGS,
     "Get a Mach port for the current task: task_self() -> int"},
    {"task_for_pid", pymach_task_for_pid, METH_VARARGS,
     "Get a Mach port for the task corresponding to a pid: task_for_pid(pid: int) -> int"},
    {"vm_protect", pymach_vm_protect, METH_VARARGS,
     "Change memory protection in another task: vm_protect(task: int, address: int, size: int, protection: int)"},
    {"vm_read", pymach_vm_read, METH_VARARGS,
     "Read memory from another task: vm_read(task, address, size) -> bytes"},
    {"vm_write", pymach_vm_write, METH_VARARGS,
     "Write memory to another task: vm_write(task: int, address: int, data: bytes)"},
    {"vm_asrl_offset", pymach_vm_asrl_offset, METH_VARARGS,
     "Get ASRL offset of another task: vm_asrl_offset(pid: int)"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "mach",
    "Wrap some low-level Mach stuff for Python 3",
    -1,
    mach_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyObject *PyInit_mach(void) {
    MachError = PyErr_NewException("mach.MachError", NULL, NULL);
    PyObject *module = PyModule_Create(&moduledef);
    PyModule_AddObject(module, "MachError", MachError);
    PyModule_AddObject(module, "VM_PROT_NONE", PyLong_FromLong(VM_PROT_NONE));
    PyModule_AddObject(module, "VM_PROT_READ", PyLong_FromLong(VM_PROT_READ));
    PyModule_AddObject(module, "VM_PROT_WRITE", PyLong_FromLong(VM_PROT_WRITE));
    PyModule_AddObject(module, "VM_PROT_EXECUTE", PyLong_FromLong(VM_PROT_EXECUTE));
    PyModule_AddObject(module, "VM_PROT_DEFAULT", PyLong_FromLong(VM_PROT_DEFAULT));
    PyModule_AddObject(module, "VM_PROT_ALL", PyLong_FromLong(VM_PROT_ALL));
    return module;
}
