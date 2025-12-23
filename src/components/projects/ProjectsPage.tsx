import { useState, useEffect } from 'react';
import { api } from '../../services/api';
import ProjectList from './ProjectList';
import ProjectForm from './ProjectForm';
import ProjectDetails from './ProjectDetails';
import './ProjectsPage.css';

type ViewType = 'list' | 'create' | 'edit' | 'details';

interface ProjectsPageProps {
  userId: string;
}

export default function ProjectsPage({ userId }: ProjectsPageProps) {
  const [view, setView] = useState<ViewType>('list');
  const [selectedProjectId, setSelectedProjectId] = useState<string | null>(null);
  const [userInfo, setUserInfo] = useState<{ sub: string; name: string; email: string } | null>(null);

  useEffect(() => {
    loadUserInfo();
  }, []);

  const loadUserInfo = async () => {
    try {
      const info = await api.getMe();
      setUserInfo(info);
    } catch (err) {
      console.error('Failed to load user info:', err);
    }
  };

  const handleProjectClick = (projectId: string) => {
    setSelectedProjectId(projectId);
    setView('details');
  };

  const handleCreateProject = () => {
    setSelectedProjectId(null);
    setView('create');
  };

  const handleEditProject = () => {
    setView('edit');
  };

  const handleSaveProject = () => {
    setView('list');
    setSelectedProjectId(null);
  };

  const handleDeleteProject = () => {
    setView('list');
    setSelectedProjectId(null);
  };

  const handleBack = () => {
    setView('list');
    setSelectedProjectId(null);
  };

  if (!userInfo) {
    return <div className="projects-page-container">Загрузка...</div>;
  }

  return (
    <div className="projects-page-container">
      {view === 'list' && (
        <ProjectList
          userId={userInfo.sub}
          onProjectClick={handleProjectClick}
          onCreateProject={handleCreateProject}
        />
      )}

      {view === 'create' && (
        <ProjectForm
          userId={userInfo.sub}
          onSave={handleSaveProject}
          onCancel={handleBack}
        />
      )}

      {view === 'edit' && selectedProjectId && (
        <ProjectForm
          userId={userInfo.sub}
          projectId={selectedProjectId}
          onSave={handleSaveProject}
          onCancel={() => setView('details')}
        />
      )}

      {view === 'details' && selectedProjectId && (
        <ProjectDetails
          projectId={selectedProjectId}
          userId={userInfo.sub}
          onEdit={handleEditProject}
          onDelete={handleDeleteProject}
          onBack={handleBack}
        />
      )}
    </div>
  );
}

