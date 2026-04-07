const button = document.getElementById("coolBtn");
const message = document.getElementById("message");

const messages = [
  "🔥 You just activated cool mode",
  "🚀 Nice click!",
  "✨ Simple. Clean. Cool.",
  "😎 You have good taste"
];

button.addEventListener("click", () => {
  const randomMessage = messages[Math.floor(Math.random() * messages.length)];
  message.textContent = randomMessage;
});