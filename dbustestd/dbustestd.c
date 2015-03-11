/*****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Simo Piiroinen <simo.piiroinen@jollamobile.com>
** All rights reserved.
**
** You may use this file under the terms of the GNU Lesser General
** Public License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file license.lgpl included in the packaging
** of this file.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
******************************************************************************/

#define TESTSRV_SERVICE   "org.nemomobile.dbustestd"
#define TESTSRV_INTERFACE "org.nemomobile.dbustestd"

#define TESTSRV_OBJ_ROOT "/"

#define TESTSRV_REQ_REPR "repr"
#define TESTSRV_REQ_ECHO "echo"
#define TESTSRV_REQ_PING "ping"
#define TESTSRV_REQ_QUIT "quit"

#define TESTSRV_SIG_PONG "pong"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>

#include <dbus/dbus-glib-lowlevel.h>

#define log_emit(LEV,FMT,ARGS...) syslog(LEV, FMT, ## ARGS)

/* ========================================================================= *
 * TYPES & FUNCTIONS
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * DBUS_HELPERS
 * ------------------------------------------------------------------------- */

typedef union
{
    dbus_int16_t   i16;
    dbus_int32_t   i32;
    dbus_int64_t   i64;

    dbus_uint16_t  u16;
    dbus_uint32_t  u32;
    dbus_uint64_t  u64;

    dbus_bool_t    b;
    unsigned char  o;
    const char    *s;
    double         d;
    int            fd;

} xdbus_any_t;

#define XDBUS_ANY_INIT { .u64 = 0 }

static bool  xdbus_message_repr_sub          (FILE *file, DBusMessageIter *iter);
static char *xdbus_message_repr              (DBusMessage *const msg);
static char *xdbus_message_iter_get_signature(DBusMessageIter *iter);
static bool  xdbus_message_copy_sub          (DBusMessageIter *dest, DBusMessageIter *srce);

/* ------------------------------------------------------------------------- *
 * SERVICE
 * ------------------------------------------------------------------------- */

typedef DBusMessage *(*service_handler_t)(DBusMessage *req);

typedef struct
{
  const char        *sm_interface;
  const char        *sm_member;
  service_handler_t  sm_handler;
} service_method_t;

static void               service_inject_dict_entry    (DBusMessageIter *arr, const char *key, int val);
static void               service_inject_dict          (DBusMessageIter *body);
static void               service_inject_array_item    (DBusMessageIter *arr, int val);
static void               service_inject_array         (DBusMessageIter *body);
static void               service_inject_struct        (DBusMessageIter *body);
static void               service_inject_variant_int32 (DBusMessageIter *body);
static DBusMessage       *service_inject_fake_args     (DBusMessage *req);

static DBusMessage       *service_handle_introspect_req(DBusMessage *req);
static DBusMessage       *service_handle_repr_req      (DBusMessage *req);
static DBusMessage       *service_handle_echo_req      (DBusMessage *req);
static DBusMessage       *service_handle_ping_req      (DBusMessage *req);
static DBusMessage       *service_handle_quit_req      (DBusMessage *req);

static service_handler_t  service_get_handler          (const char *interface, const char *member);

static DBusHandlerResult  service_filter_cb            (DBusConnection *con, DBusMessage *msg, gpointer aptr);

static bool               service_init                 (void);
static void               service_quit                 (void);

/* ------------------------------------------------------------------------- *
 * MAINLOOP
 * ------------------------------------------------------------------------- */

static void mainloop_exit(int xc);
static int  mainloop_run (void);

/* ------------------------------------------------------------------------- *
 * STAYALIVE
 * ------------------------------------------------------------------------- */

static gboolean stayalive_timer_cb(gpointer aptr);
static void     stayalive_renew   (void);
static void     stayalive_quit    (void);

/* ------------------------------------------------------------------------- *
 * ENTRY_POINT
 * ------------------------------------------------------------------------- */

int main(int ac, char **av);

/* ========================================================================= *
 * DBUS_HELPERS
 * ========================================================================= */

static bool
xdbus_message_repr_sub(FILE *file, DBusMessageIter *iter)
{
    xdbus_any_t     val = XDBUS_ANY_INIT;
    DBusMessageIter sub;

    switch( dbus_message_iter_get_arg_type(iter) ) {
    case DBUS_TYPE_INVALID:
        return false;
    default:
        fprintf(file, " unknown");
        break;
    case DBUS_TYPE_UNIX_FD:
        dbus_message_iter_get_basic(iter, &val.fd);
        fprintf(file, " fd:%d", val.fd);
        break;
    case DBUS_TYPE_BYTE:
        dbus_message_iter_get_basic(iter, &val.o);
        fprintf(file, " byte:%d", val.o);
        break;
    case DBUS_TYPE_BOOLEAN:
        dbus_message_iter_get_basic(iter, &val.b);
        fprintf(file, " boolean:%s", val.b ? "true" : "false");
        break;
    case DBUS_TYPE_INT16:
        dbus_message_iter_get_basic(iter, &val.i16);
        fprintf(file, " int16:%d", val.i16);
        break;
    case DBUS_TYPE_INT32:
        dbus_message_iter_get_basic(iter, &val.i32);
        fprintf(file, " int32:%d", val.i32);
        break;
    case DBUS_TYPE_INT64:
        dbus_message_iter_get_basic(iter, &val.i64);
        fprintf(file, " int64:%lld", (long long)val.i64);
        break;
    case DBUS_TYPE_UINT16:
        dbus_message_iter_get_basic(iter, &val.u16);
        fprintf(file, " uint16:%u", val.u16);
        break;
    case DBUS_TYPE_UINT32:
        dbus_message_iter_get_basic(iter, &val.u32);
        fprintf(file, " uint32:%u", val.u32);
        break;
    case DBUS_TYPE_UINT64:
        dbus_message_iter_get_basic(iter, &val.u64);
        fprintf(file, " uint64:%llu", (unsigned long long)val.u64);
        break;
    case DBUS_TYPE_DOUBLE:
        dbus_message_iter_get_basic(iter, &val.d);
        fprintf(file, " double:%g", val.d);
        break;
    case DBUS_TYPE_STRING:
        dbus_message_iter_get_basic(iter, &val.s);
        fprintf(file, " string:\"%s\"", val.s);
        break;
    case DBUS_TYPE_OBJECT_PATH:
        dbus_message_iter_get_basic(iter, &val.s);
        fprintf(file, " objpath:\"%s\"", val.s);
        break;
    case DBUS_TYPE_SIGNATURE:
        dbus_message_iter_get_basic(iter, &val.s);
        fprintf(file, " signature:\"%s\"", val.s);
        break;
    case DBUS_TYPE_ARRAY:
        dbus_message_iter_recurse(iter, &sub);
        fprintf(file, " array [");
        while( xdbus_message_repr_sub(file, &sub) ) {}
        fprintf(file, " ]");
        break;
    case DBUS_TYPE_VARIANT:
        dbus_message_iter_recurse(iter, &sub);
        fprintf(file, " variant");
        xdbus_message_repr_sub(file, &sub);
        break;
    case DBUS_TYPE_STRUCT:
        dbus_message_iter_recurse(iter, &sub);
        fprintf(file, " struct {");
        while( xdbus_message_repr_sub(file, &sub) ) {}
        fprintf(file, " }");
        break;
    case DBUS_TYPE_DICT_ENTRY:
        dbus_message_iter_recurse(iter, &sub);
        fprintf(file, " key");
        xdbus_message_repr_sub(file, &sub);
        fprintf(file, " val");
        xdbus_message_repr_sub(file, &sub);
        break;
    }

    return dbus_message_iter_next(iter);
}

static char *
xdbus_message_repr(DBusMessage *const msg)
{
    size_t  size = 0;
    char   *data = 0;
    FILE   *file = open_memstream(&data, &size);

    DBusMessageIter iter;
    dbus_message_iter_init(msg, &iter);
    while( xdbus_message_repr_sub(file, &iter) ) {}

    fclose(file);

    if( data && *data == ' ' )
        memmove(data, data+1, strlen(data));

    return data;
}

static char *
xdbus_message_iter_get_signature(DBusMessageIter *iter)
{
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    return dbus_message_iter_get_signature(&sub);
}

static bool
xdbus_message_copy_sub(DBusMessageIter *dest, DBusMessageIter *srce)
{
    bool        res = false;
    xdbus_any_t val = XDBUS_ANY_INIT;
    int         at  = dbus_message_iter_get_arg_type(srce);
    char       *sgn = 0;

    DBusMessageIter sub_srce, sub_dest;

    switch( at ) {
    case DBUS_TYPE_INVALID:
        goto EXIT;
    default:
        goto NEXT;
    case DBUS_TYPE_UNIX_FD:
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_BOOLEAN:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_DOUBLE:
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE:
        dbus_message_iter_get_basic(srce, &val);
        dbus_message_iter_append_basic(dest, at, &val);
        break;
    case DBUS_TYPE_ARRAY:
    case DBUS_TYPE_VARIANT:
        sgn = xdbus_message_iter_get_signature(srce);
        // fall through
    case DBUS_TYPE_STRUCT:
    case DBUS_TYPE_DICT_ENTRY:
        dbus_message_iter_recurse(srce, &sub_srce);
        if( !dbus_message_iter_open_container(dest, at, sgn, &sub_dest) )
            goto EXIT;
        while( xdbus_message_copy_sub(&sub_dest, &sub_srce) ) {}
        if( !dbus_message_iter_close_container(dest, &sub_dest) )
            goto EXIT;
        break;
    }

NEXT:
    res = dbus_message_iter_next(srce);

EXIT:
    dbus_free(sgn);

    return res;
}

/* ========================================================================= *
 * SERVICE
 * ========================================================================= */

static const char service_xml[] =
"<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\""
" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
"<node>\n"
"  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
"    <method name=\"Introspect\">\n"
"      <arg direction=\"out\" name=\"data\" type=\"s\"/>\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\"org.freedesktop.DBus.Peer\">\n"
"    <method name=\"Ping\"/>\n"
"    <method name=\"GetMachineId\">\n"
"      <arg direction=\"out\" name=\"machine_uuid\" type=\"s\" />\n"
"    </method>\n"
"  </interface>\n"
"  <interface name=\""TESTSRV_INTERFACE"\">\n"
"    <method name=\""TESTSRV_REQ_REPR"\">\n"
"      <arg direction=\"out\" name=\"args_as_string\" type=\"s\" />\n"
"    </method>\n"
"    <method name=\""TESTSRV_REQ_ECHO"\">\n"
"      <arg direction=\"out\" name=\"args_as_is\"/>\n"
"    </method>\n"
"    <method name=\""TESTSRV_REQ_PING"\">\n"
"      <arg direction=\"out\" name=\"args_as_is\" />\n"
"    </method>\n"
"    <method name=\""TESTSRV_REQ_QUIT"\"/>\n"
"    <signal name=\""TESTSRV_SIG_PONG"\">\n"
"      <arg name=\"args_to_ping_as_is\" />\n"
"    </signal>\n"
"  </interface>\n"
"</node>\n"
;

static DBusConnection *service_con = 0;

static void
service_inject_dict_entry(DBusMessageIter *arr, const char *key, int val)
{
    xdbus_any_t arg = XDBUS_ANY_INIT;
    DBusMessageIter ent;

    dbus_message_iter_open_container(arr,
                                     DBUS_TYPE_DICT_ENTRY,
                                     0,
                                     &ent);
    arg.s = key;
    dbus_message_iter_append_basic(&ent, DBUS_TYPE_STRING, &arg);

    arg.i32 = val;
    dbus_message_iter_append_basic(&ent, DBUS_TYPE_INT32, &arg);

    dbus_message_iter_close_container(arr, &ent);

}

static void
service_inject_dict(DBusMessageIter *body)
{
    DBusMessageIter arr;

    dbus_message_iter_open_container(body,
                                     DBUS_TYPE_ARRAY,
                                     DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                     DBUS_TYPE_STRING_AS_STRING
                                     DBUS_TYPE_INT32_AS_STRING
                                     DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                     &arr);

    service_inject_dict_entry(&arr, "foo", 1);
    service_inject_dict_entry(&arr, "bar", 2);
    service_inject_dict_entry(&arr, "baf", 3);

    dbus_message_iter_close_container(body, &arr);
}

static void
service_inject_array_item(DBusMessageIter *arr, int val)
{
    xdbus_any_t arg = XDBUS_ANY_INIT;
    DBusMessageIter var;

    dbus_message_iter_open_container(arr,
                                     DBUS_TYPE_VARIANT,
                                     DBUS_TYPE_INT32_AS_STRING,
                                     &var);
    arg.i32 = val;
    dbus_message_iter_append_basic(&var, DBUS_TYPE_INT32, &arg);

    dbus_message_iter_close_container(arr, &var);

}

static void
service_inject_array(DBusMessageIter *body)
{
    DBusMessageIter arr;

    dbus_message_iter_open_container(body,
                                     DBUS_TYPE_ARRAY,
                                     DBUS_TYPE_VARIANT_AS_STRING,
                                     &arr);
    service_inject_array_item(&arr, 4);
    service_inject_array_item(&arr, 5);
    service_inject_array_item(&arr, 6);
    dbus_message_iter_close_container(body, &arr);
}

static void
service_inject_struct(DBusMessageIter *body)
{
    xdbus_any_t arg = XDBUS_ANY_INIT;
    DBusMessageIter sub;

    dbus_message_iter_open_container(body,
                                     DBUS_TYPE_STRUCT,
                                     0,
                                     &sub);

    arg.o = 255;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_BYTE, &arg);

    arg.b = true;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_BOOLEAN, &arg);

    arg.i16 = 0x7fff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT16, &arg);

    arg.i32 = 0x7fffffff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT32, &arg);

    arg.i64 = 0x7fffffffffffffff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_INT64, &arg);

    arg.u16 = 0xffff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT16, &arg);

    arg.u32 = 0xffffffff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT32, &arg);

    arg.u64 = 0xffffffffffffffff;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_UINT64, &arg);

    arg.d = 3.75;
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_DOUBLE, &arg);

    arg.s = "string";
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_STRING, &arg);

    arg.s = "/obj/path";
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_OBJECT_PATH, &arg);

    arg.s = "sointu";
    dbus_message_iter_append_basic(&sub, DBUS_TYPE_SIGNATURE, &arg);

    dbus_message_iter_close_container(body, &sub);
}

static void
service_inject_variant_int32(DBusMessageIter *body)
{
    xdbus_any_t val = { .i32 = 42 };
    DBusMessageIter var;
    dbus_message_iter_open_container(body,
                                     DBUS_TYPE_VARIANT,
                                     "i", &var);
    dbus_message_iter_append_basic(&var, DBUS_TYPE_INT32, &val);
    dbus_message_iter_close_container(body, &var);
}

static DBusMessage *
service_inject_fake_args(DBusMessage *req)
{
    DBusMessage *fake = 0;
    DBusMessage *work = 0;
    char        *name = 0;
    DBusMessageIter iter;

    dbus_message_iter_init(req, &iter);

    if( dbus_message_iter_get_arg_type(&iter) !=  DBUS_TYPE_STRING )
        goto EXIT;

    dbus_message_iter_get_basic(&iter, &name);

    work = dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL);
    if( !work )
        goto EXIT;

    dbus_message_iter_init_append(work, &iter);

    if( !strcmp(name, "COMPLEX1") )
        service_inject_variant_int32(&iter);
    else if(!strcmp(name, "COMPLEX2") )
        service_inject_dict(&iter);
    else if(!strcmp(name, "COMPLEX3") )
        service_inject_array(&iter);
    else if(!strcmp(name, "COMPLEX4") )
        service_inject_struct(&iter);
    else
        goto EXIT;

    fake = work, work = 0;

EXIT:
    if( work )
        dbus_message_unref(work);

    return fake;
}

static DBusMessage *
service_handle_introspect_req(DBusMessage *req)
{
    DBusMessage *rsp = 0;
    const char  *str = service_xml;

    if( !(rsp = dbus_message_new_method_return(req)) )
        goto EXIT;

    dbus_message_append_args(rsp,
                             DBUS_TYPE_STRING, &str,
                             DBUS_TYPE_INVALID);

EXIT:
    return rsp;
}

static DBusMessage *
service_handle_repr_req(DBusMessage *req)
{
    DBusMessage *rsp = 0;
    char        *str = 0;
    DBusMessage *inj = service_inject_fake_args(req);;

    if( !(rsp = dbus_message_new_method_return(req)) )
        goto EXIT;

    if( !(str = xdbus_message_repr(inj ?: req)) )
        goto EXIT;

    dbus_message_append_args(rsp,
                             DBUS_TYPE_STRING, &str,
                             DBUS_TYPE_INVALID);

EXIT:
    free(str);

    if( inj )
        dbus_message_unref(inj);

    return rsp;
}

static DBusMessage *
service_handle_echo_req(DBusMessage *req)
{
    DBusMessage *rsp = 0;
    DBusMessage *inj = service_inject_fake_args(req);;

    DBusMessageIter src, dst;

    if( !(rsp = dbus_message_new_method_return(req)) )
        goto EXIT;

    dbus_message_iter_init(inj ?: req, &src);
    dbus_message_iter_init_append(rsp, &dst);
    while( xdbus_message_copy_sub(&dst, &src) ) {}

EXIT:
    if( inj )
        dbus_message_unref(inj);

    return rsp;
}

static DBusMessage *
service_handle_ping_req(DBusMessage *req)
{
    DBusMessage *rsp = 0;
    DBusMessage *sig = 0;
    DBusMessage *inj = service_inject_fake_args(req);;

    DBusMessageIter src, dst;

    if( !(rsp = dbus_message_new_method_return(req)) )
        goto EXIT;

    sig = dbus_message_new_signal(TESTSRV_OBJ_ROOT,
                                  TESTSRV_INTERFACE,
                                  TESTSRV_SIG_PONG);
    if( !sig )
        goto EXIT;

    dbus_message_iter_init(inj ?: req, &src);
    dbus_message_iter_init_append(sig, &dst);
    while( xdbus_message_copy_sub(&dst, &src) ) {}

    dbus_message_iter_init(inj ?: req, &src);
    dbus_message_iter_init_append(rsp, &dst);
    while( xdbus_message_copy_sub(&dst, &src) ) {}

EXIT:
    // send signal 1st, then reply -> client should
    // have gotten the sig when they get the reply
    if( sig )  {
        dbus_connection_send(service_con, sig, 0);
        dbus_message_unref(sig);
    }

    if( inj )
        dbus_message_unref(inj);

    return rsp;
}

static DBusMessage *
service_handle_quit_req(DBusMessage *req)
{
    DBusMessage *rsp = dbus_message_new_method_return(req);

    mainloop_exit(EXIT_SUCCESS);

    return rsp;
}

static const service_method_t service_method_lut[] =
{
  {
    .sm_interface = TESTSRV_INTERFACE,
    .sm_member    = TESTSRV_REQ_REPR,
    .sm_handler   = service_handle_repr_req,
  },
  {
    .sm_interface = TESTSRV_INTERFACE,
    .sm_member    = TESTSRV_REQ_ECHO,
    .sm_handler   = service_handle_echo_req,
  },
  {
    .sm_interface = TESTSRV_INTERFACE,
    .sm_member    = TESTSRV_REQ_PING,
    .sm_handler   = service_handle_ping_req,
  },
  {
    .sm_interface = TESTSRV_INTERFACE,
    .sm_member    = TESTSRV_REQ_QUIT,
    .sm_handler   = service_handle_quit_req,
  },
  {
    .sm_interface = "org.freedesktop.DBus.Introspectable",
    .sm_member    = "Introspect",
    .sm_handler   = service_handle_introspect_req,
  },
};

static service_handler_t
service_get_handler(const char *interface, const char *member)
{
    service_handler_t handler = 0;

    for( size_t i = 0; i < G_N_ELEMENTS(service_method_lut); ++i ) {
        // test member name first because they are shorter and more
        // likely to be unique than interface names
        if( strcmp(service_method_lut[i].sm_member, member) )
            continue;

        if( strcmp(service_method_lut[i].sm_interface, interface) )
            continue;

        handler = service_method_lut[i].sm_handler;
        break;
    }

    return handler;
}

static DBusHandlerResult
service_filter_cb(DBusConnection *con, DBusMessage *msg, gpointer aptr)
{
    (void)aptr;

    DBusHandlerResult  res = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    DBusMessage       *rsp = 0;

    int type = dbus_message_get_type(msg);

    if( type != DBUS_MESSAGE_TYPE_METHOD_CALL )
        goto EXIT;

    const char *interface = dbus_message_get_interface(msg);

    if( !interface )
        goto EXIT;

    const char *member = dbus_message_get_member(msg);

    if( !member )
        goto EXIT;

    service_handler_t handler = service_get_handler(interface, member);

    if( !handler )
        goto EXIT;

    log_emit(LOG_NOTICE, "handle %s.%s()", interface, member);

    if( !(rsp = handler(msg)) )
        rsp = dbus_message_new_error(msg, DBUS_ERROR_FAILED, "internal error");

    stayalive_renew();

EXIT:
    if( rsp ) {
        res = DBUS_HANDLER_RESULT_HANDLED;
        if( !dbus_message_get_no_reply(msg) )
            dbus_connection_send(con, rsp, 0);
        dbus_message_unref(rsp);
    }

    return res;
}

static bool
service_init(void)
{
    bool        res = false;
    DBusBusType bus = DBUS_BUS_STARTER;
    DBusError   err = DBUS_ERROR_INIT;

    if( !(service_con = dbus_bus_get(bus, &err)) ) {
        log_emit(LOG_CRIT, "bus connect failed: %s: %s",
                 err.name, err.message);
        goto EXIT;
    }

    dbus_connection_setup_with_g_main(service_con, NULL);

    if( !dbus_connection_add_filter(service_con, service_filter_cb, 0,0) ) {
        log_emit(LOG_CRIT, "add message filter failed");
        goto EXIT;
    }

    int rc = dbus_bus_request_name(service_con, TESTSRV_SERVICE, 0, &err);

    if( dbus_error_is_set(&err) ) {
        log_emit(LOG_CRIT, "acquire name failed: %s: %s",
                 err.name, err.message);
        goto EXIT;
    }

    if( rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER ) {
        log_emit(LOG_CRIT, "acquire name failed: %s",
                 "not primary owner");
        goto EXIT;
    }

    res = true;

EXIT:
    dbus_error_free(&err);

    return res;
}

static void
service_quit(void)
{
    if( service_con ) {
        dbus_connection_remove_filter(service_con,
                                      service_filter_cb, 0);

        dbus_connection_flush(service_con);

        dbus_connection_unref(service_con),
            service_con = NULL;
    }
}

/* ========================================================================= *
 * MAINLOOP
 * ========================================================================= */

static GMainLoop *mainloop_hnd = 0;

static int mainloop_res = EXIT_SUCCESS;

static void
mainloop_exit(int xc)
{
    if( mainloop_res < xc )
        mainloop_res = xc;

    if( !mainloop_hnd )
        exit(mainloop_res);

    g_main_loop_quit(mainloop_hnd);
}

static int
mainloop_run(void)
{
    mainloop_res = EXIT_SUCCESS;
    mainloop_hnd = g_main_loop_new(0, 0);

    g_main_loop_run(mainloop_hnd);

    g_main_loop_unref(mainloop_hnd),
        mainloop_hnd = 0;

    return mainloop_res;
}

/* ========================================================================= *
 * STAYALIVE
 * ========================================================================= */

static guint stayalive_timer_ms = 5 * 1000;

static guint stayalive_timer_id = 0;

static gboolean
stayalive_timer_cb(gpointer aptr)
{
    if( !stayalive_timer_id )
        goto EXIT;

    stayalive_timer_id = 0;

    log_emit(LOG_NOTICE, "stayalive timeout");

    mainloop_exit(EXIT_SUCCESS);

EXIT:
    return FALSE;
}

static void
stayalive_renew(void)
{
    if( stayalive_timer_id )
        g_source_remove(stayalive_timer_id);

    stayalive_timer_id = g_timeout_add(stayalive_timer_ms,
                                       stayalive_timer_cb, 0);
}

static void
stayalive_quit(void)
{
    if( stayalive_timer_id ) {
        g_source_remove(stayalive_timer_id),
            stayalive_timer_id = 0;
    }
}

/* ========================================================================= *
 * ENTRY_POINT
 * ========================================================================= */

int
main(int ac, char **av)
{
    int xc = EXIT_FAILURE;

    log_emit(LOG_NOTICE, "init");

    if( !service_init() )
        goto EXIT;

    stayalive_renew();

    xc = mainloop_run();

EXIT:
    stayalive_quit();

    service_quit();

    log_emit(LOG_NOTICE, "exit %d", xc);
    return xc;
}
