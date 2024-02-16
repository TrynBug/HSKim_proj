using MySql.Data.MySqlClient;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;
using System.Collections;
using System.Data;
using System.Threading;
using Org.BouncyCastle.Crypto.Modes.Gcm;

namespace Server.Game
{
    // 비동기 Insert, Delete, Update 완료시 호출할 Callback
    public delegate void NonQueryCompletionCallback(int affectedRows, params object[] callbackParams);
    // 비동기 Select 완료시 호출할 Callback
    public delegate void ReaderCompletionCallback(MySqlDataReader reader, params object[] callbackParams);

    public class DBWriter
    {
        public int CurrentJobCount { get { return _jobQueue.Count; } }
        public long ExecutedJobCount { get { return _executedJobCount; } }


        // room
        RoomBase _room = null;

        // DB
        MySqlConnection _conn = null;
        MySqlDataReader _reader = null;

        // job
        object _lock = new object();
        Queue<Job> _jobQueue = new Queue<Job>();
        bool _isFlushTaskRunning = false;
        long _executedJobCount = 0;




        /* 비동기 DB job */
        abstract class Job
        {
            public MySqlCommand Command;
            public Job(MySqlCommand command)
            {
                Command = command;
            }
            public abstract void Execute();
        }

        class JobNonQuery : Job
        {
            public NonQueryCompletionCallback Callback;
            public object[] CallbackParams;
            public JobNonQuery(MySqlCommand command, NonQueryCompletionCallback callback, object[] callbackParams) : base(command)
            {
                Callback = callback;
                CallbackParams = callbackParams;
            }
            public override void Execute()
            {
                int result = Command.ExecuteNonQuery();
                if (Callback != null)
                    Callback.Invoke(result, CallbackParams);
            }
        }

        class JobReader : Job
        {
            public ReaderCompletionCallback Callback;
            public object[] CallbackParams;
            public JobReader(MySqlCommand command, ReaderCompletionCallback callback, object[] callbackParams) : base(command)
            {
                Callback = callback;
                CallbackParams = callbackParams;
            }
            public override void Execute()
            {
                MySqlDataReader reader = Command.ExecuteReader();
                if (Callback != null)
                    Callback.Invoke(reader, CallbackParams);
                reader.Close();
            }
        }



        void CloseReader()
        {
            if(_reader != null)
            {
                if (_reader.IsClosed == false)
                    _reader.Close();
                _reader = null;
            }
        }

        // connect
        public bool Connect(RoomBase room)
        {
            if (room == null)
                return false;
            _room = room;

            try
            {
                _conn = new MySqlConnection("Server=localhost;Port=3306;Database=unityaccount;Uid=root;Pwd=vmfhzkepal!");
                _conn.Open();
                Logger.WriteLog(LogLevel.System, $"DBWriter.Connect. room:{_room}");
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"DBWriter.Connect. error. room:{_room}, error:{ex}");
                return false;
            }

            return true;
        }


        // Insert, Update, Delete 쿼리 동기 실행
        public int ExecuteNonQuerySync(string query, List<MySqlParameter> parameters = null)
        {
            CloseReader();
            try
            {
                MySqlCommand command = new MySqlCommand(query, _conn);
                if (parameters != null)
                {
                    foreach(MySqlParameter param in parameters)
                        command.Parameters.Add(param);
                }

                int result = command.ExecuteNonQuery();
                _executedJobCount++;
                if (result < 0)
                {
                    Logger.WriteLog(LogLevel.Error, $"DBWriter.ExecuteQuerySync. result is -1. room:{_room}, result:{result}, query:{query}");
                }
                return result;
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"DBWriter.ExecuteQuerySync. room:{_room}, query:{query}, error:{ex}");
                return -1;
            }
        }

        // select 쿼리 동기 실행
        public IEnumerable<MySqlDataReader> ExecuteReaderSync(string selectQuery, List<MySqlParameter> parameters)
        {
            CloseReader();
            MySqlCommand command = new MySqlCommand(selectQuery, _conn);
            try
            {
                if (parameters != null)
                {
                    foreach (MySqlParameter param in parameters)
                        command.Parameters.Add(param);
                }

                _reader = command.ExecuteReader();
                _executedJobCount++;
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"DBWriter.ExecuteReaderSync. {_room}, query:{command.CommandText}, error:{ex}");
                CloseReader();
            }

            // yield
            if (_reader != null)
            {
                while (_reader.Read())
                {
                    yield return _reader;
                }
                CloseReader();
            }
        }

        // select 쿼리 동기 실행
        public IEnumerable<MySqlDataReader> ExecuteReaderSync(string selectQuery, MySqlParameter param = null)
        {
            CloseReader();
            MySqlCommand command = new MySqlCommand(selectQuery, _conn);
            try
            {
                if (param != null)
                {
                    command.Parameters.Add(param);
                }

                _reader = command.ExecuteReader();
                _executedJobCount++;
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"DBWriter.ExecuteReaderSync. {_room}, query:{command.CommandText}, error:{ex}");
                CloseReader();
            }

            // yield
            if (_reader != null)
            {
                while (_reader.Read())
                {
                    yield return _reader;
                }
                CloseReader();
            }
        }












        // Insert, Update, Delete 쿼리 비동기 실행
        public void ExecuteNonQueryAsync(string query, List<MySqlParameter> parameters, NonQueryCompletionCallback callback, params object[] callbackParams)
        {
            // command 생성
            MySqlCommand command = new MySqlCommand(query, _conn);
            if (parameters != null)
            {
                foreach (MySqlParameter param in parameters)
                    command.Parameters.Add(param);
            }

            // job 등록
            JobNonQuery job = new JobNonQuery(command, callback, callbackParams);
            bool bFlush = false;
            lock (_lock)
            {
                _jobQueue.Enqueue(job);
                if (_isFlushTaskRunning == false)
                {
                    _isFlushTaskRunning = true;
                    bFlush = true;
                }
            }

            // 만약 현재 flush 중이 아니라면 flush 한다.
            if (bFlush)
                FlushJob();
        }



        // Insert, Update, Delete 쿼리 비동기 실행
        public void ExecuteReaderAsync(string query, List<MySqlParameter> parameters, ReaderCompletionCallback callback, params object[] callbackParams)
        {
            // command 생성
            MySqlCommand command = new MySqlCommand(query, _conn);
            if (parameters != null)
            {
                foreach (MySqlParameter param in parameters)
                    command.Parameters.Add(param);
            }

            // job 등록
            JobReader job = new JobReader(command, callback, callbackParams);
            bool bFlush = false;
            lock (_lock)
            {
                _jobQueue.Enqueue(job);
                if (_isFlushTaskRunning == false)
                {
                    _isFlushTaskRunning = true;
                    bFlush = true;
                }
            }

            // 만약 현재 flush 중이 아니라면 flush 한다.
            if (bFlush)
                FlushJob();
        }


        // jobQueue 내의 모든 job을 실행하는 task를 시작한다.
        // task는 한 시점에 1개의 스레드에서만 실행되고 있어야 한다.
        // 이를 위해 _isFlushTaskRunning 를 true로 세팅하고 이 함수를 호출해야 한다.
        void FlushJob()
        {
            Task task = new Task(() =>
            {
                Job job;
                while (true)
                {
                    lock (_lock)
                    {
                        if (_jobQueue.Count == 0)
                        {
                            _isFlushTaskRunning = false;
                            break;
                        }
                        job = _jobQueue.Dequeue();
                    }
                    job.Execute();
                    _executedJobCount++;
                }
            });
            task.Start();
        }
    }
}
