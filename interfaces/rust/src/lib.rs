/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #![allow(missing_docs)]

use std::ffi::{c_char, c_void};

type XCollieCallbackRust = extern "C" fn(arg: *mut c_void);

extern "C" {
#[allow(unused)]
    pub fn SetTimerRust(
        data: *const c_char,
        timeout: u32,
        func: XCollieCallbackRust,
        arg: *mut c_void,
        flag: u32,
    ) -> i32;

#[allow(unused)]
    fn CancelTimerRust(id: i32);
}

/**
* # Safety
*/
#[allow(unused)]
pub unsafe fn set_timer(
    data: *const c_char,
    timeout: u32,
    func: XCollieCallbackRust,
    arg: *mut c_void,
    flag: u32,
) -> i32 {
    unsafe { SetTimerRust(data, timeout, func, arg, flag) }
}

#[allow(unused)]
pub fn cancel_timer(id: i32) {
    unsafe { CancelTimerRust(id) };
}
