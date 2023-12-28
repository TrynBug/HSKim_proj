using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServerCore
{
    public enum LogLevel
    {
        Debug,
        System,
        Error,
        None,
    }

    public static class Logger
    {
        public static LogLevel Level = LogLevel.System;
        public static Action<LogLevel, string>? OnLog = null;

        public static void WriteLog(LogLevel level, string log)
        {
            if (level < Level)
                return;
            if (Level == LogLevel.None)
                return;

            string outLog;
            switch(level)
            {
                // 여기서 문자열 보간($)을 사용하지 않은 이유는 log 자체에 { } 문자가 포함되어 있을 수 있기 때문이다.
                case LogLevel.Debug:
                    outLog = string.Format("{0}  DEBUG   {1}", DateTime.Now.ToString("yyyy-mm-dd HH:mm:ss:ff"), log);
                    break;
                case LogLevel.System:
                    outLog = string.Format("{0}  SYSTEM  {1}", DateTime.Now.ToString("yyyy-mm-dd HH:mm:ss:ff"), log);
                    break;
                case LogLevel.Error:
                    outLog = string.Format("{0}  ERROR   {1}", DateTime.Now.ToString("yyyy-mm-dd HH:mm:ss:ff"), log);
                    break;
                default:
                    outLog = string.Format("{0}  NONE    {1}", DateTime.Now.ToString("yyyy-mm-dd HH:mm:ss:ff"), log);
                    break;
            }

            if (OnLog == null)
                Console.WriteLine(outLog);
            else
                OnLog.Invoke(level, outLog);
        }

    }
}
