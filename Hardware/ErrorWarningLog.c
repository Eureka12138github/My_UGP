#include "ErrorWarningLog.h"
/**
 * @brief 将指定索引的错误时间信息存储到 Store_Data 数组中
 *
 * 该函数根据传入的索引值，将 `ErrorTime` 数组中对应索引的错误时间信息
 * 存储到 `Store_Data` 数组中。存储的起始索引由 `STORE_DATA_START_INDEX`
 * 宏定义，并且每个错误时间信息占用 9 个字节。
 *
 * @param index 要存储的错误时间信息的索引值（范围：0 到 ERROR_TIME_ARRAY_SIZE-1）
 */
void StoreErrorTime(u8 index) {
    // 计算存储起始索引
    u16 baseIndex = STORE_ERROR_DATA_START_INDEX + index * 9;

    // 存储年份
    Store_Data[baseIndex + 0] = ErrorTime[index].errortime.Year;
    // 存储月份
    Store_Data[baseIndex + 1] = ErrorTime[index].errortime.Month;
    // 存储日期
    Store_Data[baseIndex + 2] = ErrorTime[index].errortime.Day;
    // 存储小时
    Store_Data[baseIndex + 3] = ErrorTime[index].errortime.Hour;
    // 存储分钟
    Store_Data[baseIndex + 4] = ErrorTime[index].errortime.Minute;
    // 存储秒
    Store_Data[baseIndex + 5] = ErrorTime[index].errortime.Second;
    // 存储星期几
    Store_Data[baseIndex + 6] = ErrorTime[index].errortime.wday;
    // 存储错误类型
    Store_Data[baseIndex + 7] = ErrorTime[index].errortype;
    // 存储错误显示标志
    Store_Data[baseIndex + 8] = ErrorTime[index].errorshowflag;
}
void StoreWarningTime(u8 index) {
    // 计算存储起始索引
    u16 baseIndex = STORE_WARNING_DATA_START_INDEX + index * 9;

    // 存储年份
    Store_Data[baseIndex + 0] = WarningTime[index].warningtime.Year;
    // 存储月份
    Store_Data[baseIndex + 1] = WarningTime[index].warningtime.Month;
    // 存储日期
    Store_Data[baseIndex + 2] = WarningTime[index].warningtime.Day;
    // 存储小时
    Store_Data[baseIndex + 3] = WarningTime[index].warningtime.Hour;
    // 存储分钟
    Store_Data[baseIndex + 4] = WarningTime[index].warningtime.Minute;
    // 存储秒
    Store_Data[baseIndex + 5] = WarningTime[index].warningtime.Second;
    // 存储星期几
    Store_Data[baseIndex + 6] = WarningTime[index].warningtime.wday;
    // 存储错误类型
    Store_Data[baseIndex + 7] = WarningTime[index].warningtype;
    // 存储错误显示标志
    Store_Data[baseIndex + 8] = WarningTime[index].warningshowflag;
}
void ReadStoreWarningTime(void) {
    u16 baseIndex = 0;
    for (u8 i = 0; i < 3; i++) {
        // 计算当前错误时间信息的起始索引
        baseIndex = STORE_WARNING_DATA_START_INDEX + i * 9;

        // 检查 errorshowflag 是否为 1，表示该条记录有效
        if (Store_Data[baseIndex + 8] == 1) {
            // 读取并存储年份
            WarningTime[i].warningtime.Year = Store_Data[baseIndex + 0];
            // 读取并存储月份
            WarningTime[i].warningtime.Month = Store_Data[baseIndex + 1];
            // 读取并存储日期
            WarningTime[i].warningtime.Day = Store_Data[baseIndex + 2];
            // 读取并存储小时
            WarningTime[i].warningtime.Hour = Store_Data[baseIndex + 3];
            // 读取并存储分钟
            WarningTime[i].warningtime.Minute = Store_Data[baseIndex + 4];
            // 读取并存储秒
            WarningTime[i].warningtime.Second = Store_Data[baseIndex + 5];
            // 读取并存储星期几
            WarningTime[i].warningtime.wday = Store_Data[baseIndex + 6];
            // 读取并存储错误类型
            WarningTime[i].warningtype = Store_Data[baseIndex + 7];
            // 读取并存储错误显示标志
            WarningTime[i].warningshowflag = Store_Data[baseIndex + 8];
        }
    }
}
/**
 * @brief 从 Store_Data 数组中读取错误时间信息到 ErrorTime 数组
 *
 * 该函数从 `Store_Data` 数组中读取存储的错误时间信息，并将其加载到
 * `ErrorTime` 数组中。每个错误时间信息占用 9 个字节，存储的起始索引
 * 由 `STORE_DATA_START_INDEX` 宏定义。
 *
 * 读取逻辑如下：
 * - 计算每个错误时间信息的起始索引。
 * - 检查 `errorshowflag` 是否为 1，如果是，则读取并存储相应的错误时间信息。
 */
void ReadStoreErrorTime(void) {
    u16 baseIndex = 0;
    for (u8 i = 0; i < 3; i++) {
        // 计算当前错误时间信息的起始索引
        baseIndex = STORE_ERROR_DATA_START_INDEX + i * 9;

        // 检查 errorshowflag 是否为 1，表示该条记录有效
        if (Store_Data[baseIndex + 8] == 1) {
            // 读取并存储年份
            ErrorTime[i].errortime.Year = Store_Data[baseIndex + 0];
            // 读取并存储月份
            ErrorTime[i].errortime.Month = Store_Data[baseIndex + 1];
            // 读取并存储日期
            ErrorTime[i].errortime.Day = Store_Data[baseIndex + 2];
            // 读取并存储小时
            ErrorTime[i].errortime.Hour = Store_Data[baseIndex + 3];
            // 读取并存储分钟
            ErrorTime[i].errortime.Minute = Store_Data[baseIndex + 4];
            // 读取并存储秒
            ErrorTime[i].errortime.Second = Store_Data[baseIndex + 5];
            // 读取并存储星期几
            ErrorTime[i].errortime.wday = Store_Data[baseIndex + 6];
            // 读取并存储错误类型
            ErrorTime[i].errortype = Store_Data[baseIndex + 7];
            // 读取并存储错误显示标志
            ErrorTime[i].errorshowflag = Store_Data[baseIndex + 8];
        }
    }
}
/**
 * @brief 记录错误类型并存储错误时间信息
 *
 * 该函数根据传入的错误类型记录错误信息，并将其存储到 `ErrorTime` 数组中。
 * 如果错误类型不为 0，则记录当前时间并设置错误显示标志。随后，将记录的信息存储到
 * `Store_Data` 数组中，并保存到 Flash 存储中。`LogIndex` 用于跟踪当前记录的位置，
 * 并在达到数组大小时循环重置。
 *
 * @param Type 错误类型（范围：0 到 255）实际范围没那么大，根据需求填入
 */
void ErrorType(u8 Type) {
    // 使用局部变量代替静态变量，每次运行都从已使用的下一个位置开始
    u8 LogIndex = 0;
    
    // 查找下一个可用的存储位置
    // 按照您的思路，检查 errorshowflag 标志来确定写入位置
    for(u8 i = 0; i < ERROR_TIME_ARRAY_SIZE; i++) {
        if(!ErrorTime[i].errorshowflag) {
            LogIndex = i;
            break;
        }
        // 如果所有位置都被占用，则覆盖最旧的数据（索引0）
        if(i == ERROR_TIME_ARRAY_SIZE - 1) {
            LogIndex = 0;
        }
    }

    // 如果错误类型不为 0，则记录错误信息
    if (Type != 0) {
        // 设置错误类型
        ErrorTime[LogIndex].errortype = Type;
        // 设置错误显示标志为 true
        ErrorTime[LogIndex].errorshowflag = true;

        // 读取当前时间
        MyRTC_ReadTime();
        // 记录当前时间信息
        ErrorTime[LogIndex].errortime.Year = Time.Year;
        ErrorTime[LogIndex].errortime.Month = Time.Month;
        ErrorTime[LogIndex].errortime.Day = Time.Day;
        ErrorTime[LogIndex].errortime.Hour = Time.Hour;
        ErrorTime[LogIndex].errortime.Minute = Time.Minute;
        ErrorTime[LogIndex].errortime.Second = Time.Second;
        ErrorTime[LogIndex].errortime.wday = Time.wday;

        // 将错误时间信息存储到 Store_Data 数组中
        StoreErrorTime(LogIndex);
        // 保存到 Flash 存储中
        Store_Save();
        // 注意：不再更新 LogIndex，因为下次调用时会重新查找可用位置
    }
}
/**
 * @brief 记录警报类型并存储警报时间信息
 *
 * 该函数根据传入的错误类型记录警报信息，并将其存储到 `WarningTime` 数组中。
 * 如果警报类型不为 0，则记录当前时间并设置警报显示标志。随后，将记录的信息存储到
 * `Store_Data` 数组中，并保存到 Flash 存储中。`LogIndex` 用于跟踪当前记录的位置，
 * 并在达到数组大小时循环重置。
 *
 * @param Type 错误类型（范围：0 到 255）实际范围没那么大，根据需求填入
 */
void WarningType(u8 Type) {
    // 静态变量，用于跟踪当前记录的位置
    static u8 LogIndex = 0;

    // 如果错误类型不为 0，则记录错误信息
    if (Type != 0) {
        // 设置错误类型
        WarningTime[LogIndex].warningtype = Type;
        // 设置错误显示标志为 true
        WarningTime[LogIndex].warningshowflag = true;

        // 读取当前时间
        MyRTC_ReadTime();
        // 记录当前时间信息
        WarningTime[LogIndex].warningtime.Year = Time.Year;
        WarningTime[LogIndex].warningtime.Month = Time.Month;
        WarningTime[LogIndex].warningtime.Day = Time.Day;
        WarningTime[LogIndex].warningtime.Hour = Time.Hour;
        WarningTime[LogIndex].warningtime.Minute = Time.Minute;
        WarningTime[LogIndex].warningtime.Second = Time.Second;
        WarningTime[LogIndex].warningtime.wday = Time.wday;

        // 将错误时间信息存储到 Store_Data 数组中
//        StoreWarningTime(LogIndex);
//        // 保存到 Flash 存储中
//        Store_Save();
        // 更新 LogIndex，循环重置
        LogIndex = (LogIndex + 1) % WARNING_TIME_ARRAY_SIZE;          
    }
}
