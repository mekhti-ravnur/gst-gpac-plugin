#pragma once

#define SET_PROP(prop_4cc, value)                                            \
  gpac_return_val_if_fail(gf_filter_pid_set_property(pid, prop_4cc, &value), \
                          FALSE);

#define SKIP_IF_SET(prop_4cc)                      \
  do {                                             \
    if (gf_filter_pid_get_property(pid, prop_4cc)) \
      return TRUE;                                 \
  } while (0);
