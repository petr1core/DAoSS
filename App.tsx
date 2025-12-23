export default function App() {
  // Redirect to index.html
  if (typeof window !== 'undefined') {
    window.location.href = '/index.html';
  }
  
  return (
    <div style={{ 
      display: 'flex', 
      alignItems: 'center', 
      justifyContent: 'center', 
      height: '100vh',
      flexDirection: 'column',
      gap: '1rem'
    }}>
      <h1>Редактор блок-схем</h1>
      <p>Переадресация на HTML версию...</p>
      <a href="/index.html" style={{ color: '#3b82f6', textDecoration: 'underline' }}>
        Нажмите здесь, если переадресация не работает
      </a>
    </div>
  );
}
