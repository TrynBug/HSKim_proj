using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class HealthComponent : MonoBehaviour
{
    public UnityAction<float> OnHealthChanged { get; set; }
    public UnityAction OnHealthEmpty { get; set; }

    [Tooltip("The max amount of health that can be assigned")]
    [SerializeField]
    private float maxHealth = 100.0f;

    [Tooltip("The initial amount of health assigned")]
    [SerializeField, Range(0, 1)]
    private float initialRatio = 1.0f;

    /// <summary>
    /// Gets or sets the max amount of health
    /// </summary>
    public float MaxHealth { get => maxHealth; set => maxHealth = value; }

    /// <summary>
    /// Gets the current amount of health
    /// </summary>
    public float CurrentHealth { get; set; } = 0.0f;

    /// <summary>
    /// Returns true if the current amount of health is greater than zero
    /// </summary>
    public bool IsAlive => CurrentHealth > 0.0f;

    private void Awake()
    {
        SetHealth(maxHealth * initialRatio);
    }

    /// <summary>
    /// Sets the given amount of health
    /// </summary>
    /// <param name="health"></param>
    public void SetHealth(float health)
    {
        float previousHealth = CurrentHealth;

        CurrentHealth = Mathf.Clamp(health, 0, MaxHealth);

        float difference = health - previousHealth;

        if (difference > 0.0f)
        {
            OnHealthChanged?.Invoke(difference);
        }
    }

    /// <summary>
    /// Adds the given amount of health
    /// </summary>
    /// <param name="amount"></param>
    public void AddHealth(float amount)
    {
        if (IsAlive == false)
        {
            return;
        }

        float previousHealth = CurrentHealth;

        CurrentHealth += amount;

        CurrentHealth = Mathf.Clamp(CurrentHealth, 0, MaxHealth);

        float changeAmount = CurrentHealth - previousHealth;

        if (changeAmount > 0.0f)
        {
            OnHealthChanged?.Invoke(changeAmount);
        }
    }

    /// <summary>
    /// Applies the given amount of damage
    /// </summary>
    /// <param name="damage"></param>
    public void ApplyDamage(float damage)
    {
        if (IsAlive == false)
        {
            return;
        }

        float previousHealth = CurrentHealth;

        CurrentHealth = Mathf.Clamp(CurrentHealth - damage, 0, MaxHealth);

        float changeAmount = CurrentHealth - previousHealth;

        if (Mathf.Abs(changeAmount) > 0.0f)
        {
            OnHealthChanged?.Invoke(changeAmount);

            if (CurrentHealth <= 0.0f)
            {
                OnHealthEmpty?.Invoke();
            }
        }
    }
}
