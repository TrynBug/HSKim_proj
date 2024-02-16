using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient.Game
{
    // Job을 하나의 스레드에서 동기화 없이 수행하기 위한 JobSerializer 이다.
    // Push 함수를 통해 Action 형태로 Job을 삽입한다.
    // JobSerializer를 소유하고 있는 객체(GameRoom 등)가 주기적으로(Update 함수 등에서) Flush를 호출하여 Job을 실행해주어야 한다.
    public class JobSerializer
    {
        Queue<IJob> _jobQueue = new Queue<IJob>();
        JobTimer _timer = new JobTimer();
        object _lock = new object();
        bool _flush = false;

        public int JobCount { get { return _jobQueue.Count; } }


        // job queue에 job을 삽입
        public void Push(IJob job)
        {
            lock (_lock)
            {
                _jobQueue.Enqueue(job);
            }
        }

        public void Push(Action action) { Push(new Job(action)); }
        public void Push<T1>(Action<T1> action, T1 t1) { Push(new Job<T1>(action, t1)); }
        public void Push<T1, T2>(Action<T1, T2> action, T1 t1, T2 t2) { Push(new Job<T1, T2>(action, t1, t2)); }
        public void Push<T1, T2, T3>(Action<T1, T2, T3> action, T1 t1, T2 t2, T3 t3) { Push(new Job<T1, T2, T3>(action, t1, t2, t3)); }


        // tickAfter 이후에 job을 실행
        public void PushAfter(int tickAfter, IJob job)
        {
            _timer.Push(job, tickAfter);
        }

        public void PushAfter(int tickAfter, Action action) { PushAfter(tickAfter, new Job(action)); }
        public void PushAfter<T1>(int tickAfter, Action<T1> action, T1 t1) { PushAfter(tickAfter, new Job<T1>(action, t1)); }
        public void PushAfter<T1, T2>(int tickAfter, Action<T1, T2> action, T1 t1, T2 t2) { PushAfter(tickAfter, new Job<T1, T2>(action, t1, t2)); }
        public void PushAfter<T1, T2, T3>(int tickAfter, Action<T1, T2, T3> action, T1 t1, T2 t2, T3 t3) { PushAfter(tickAfter, new Job<T1, T2, T3>(action, t1, t2, t3)); }

        // job queue, job timer 내의 모든 job을 실행한다.
        public void Flush()
        {
            _timer.Flush();
            while (true)
            {
                IJob job = Pop();
                if (job == null)
                    return;

                job.Execute();
            }
        }



        // pop
        private IJob Pop()
        {
            lock (_lock)
            {
                if (_jobQueue.Count == 0)
                {
                    // _jobQueue에 job이 없다면 _flush를 false로 만들어둔다.
                    // 그러면 다음번에 Push하는 스레드가 _jobQueue내의 모든 job을 실행하게된다.
                    _flush = false;
                    return null;
                }

                return _jobQueue.Dequeue();
            }
        }


    }
}
