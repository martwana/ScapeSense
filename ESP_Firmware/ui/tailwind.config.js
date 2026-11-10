export default {
  content: ['./index.html', './src/**/*.{js,jsx}'],
  theme: {
    extend: {
      colors: {
        abyss: 'rgb(var(--abyss) / <alpha-value>)',
        reef: 'rgb(var(--reef) / <alpha-value>)',
        lagoon: 'rgb(var(--lagoon) / <alpha-value>)',
        foam: 'rgb(var(--foam) / <alpha-value>)',
        aqua: 'rgb(var(--aqua) / <alpha-value>)',
        coral: 'rgb(var(--coral) / <alpha-value>)',
        amber: 'rgb(var(--amber) / <alpha-value>)',
        white: 'rgb(var(--ink) / <alpha-value>)',
      },
      fontFamily: {
        sans: ['Inter', 'ui-sans-serif', 'system-ui', 'sans-serif'],
      },
      boxShadow: {
        glow: '0 0 40px rgba(49, 208, 170, 0.12)',
      },
    },
  },
  plugins: [],
};
