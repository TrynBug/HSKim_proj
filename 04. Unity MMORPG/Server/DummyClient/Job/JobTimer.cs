using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using ServerCore;

namespace DummyClient.Game
{
    struct JobTimerElem : IComparable<JobTimerElem>
    {
        public int execTick;   // 다음 실행시간
        public IJob job;       // Job

        // 내 execTick이 상대 execTick보다 작으면(상대보다 더 빨리 실행되어야 하면) 우선순위가 높도록 비교
        public int CompareTo(JobTimerElem other)
        {
            return other.execTick - execTick;
        }
    }

    // Job을 보관하는 우선순위 큐를 가지며, Job의 다음 실행시간(execTick)이 가장 작은 Job이 우선순위가 가장 높다.
    // Flush 함수가 호출되면 큐 내의 실행시간이 도달한 모든 Job이 실행된다.
    // Flush 함수를 누군가 주기적으로 호출해주어야 한다.
    public class JobTimer
    {
        PriorityQueue<JobTimerElem> _pq = new PriorityQueue<JobTimerElem>();
        object _lock = new object();

        public static JobTimer Instance { get; } = new JobTimer();

        // Job 삽입
        // job : 실행할 Job
        // tickAfter : 몇 tick 후에 실행될지를 설정
        public void Push(IJob job, int tickAfter = 0)
        {
            // Job 타이머 생성
            JobTimerElem jobElm;
            jobElm.execTick = System.Environment.TickCount + tickAfter;
            jobElm.job = job;

            lock(_lock)
            {
                _pq.Push(jobElm);
            }
        }

        // Job 실행
        public void Flush()
        {
            while(true)
            {
                int now = System.Environment.TickCount;

                JobTimerElem jobElm;
                lock(_lock)
                {
                    if (_pq.Count == 0)
                        break;

                    // Job 중 실행시간이 된 Job이 없다면 break
                    jobElm = _pq.Peek();
                    if (jobElm.execTick > now)
                        break;
                    _pq.Pop();
                }

                // Job 실행
                jobElm.job.Execute();
            }
        }
    }
}
