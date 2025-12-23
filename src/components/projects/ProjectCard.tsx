import type { Project } from '../../services/api';
import './ProjectCard.css';

interface ProjectCardProps {
  project: Project;
  onClick: () => void;
}

export default function ProjectCard({ project, onClick }: ProjectCardProps) {
  const formatDate = (dateString: string) => {
    const date = new Date(dateString);
    return date.toLocaleDateString('ru-RU', {
      year: 'numeric',
      month: 'long',
      day: 'numeric',
    });
  };

  return (
    <div className="project-card" onClick={onClick}>
      <h3 className="project-card-title">{project.name}</h3>
      <p className="project-card-description">{project.description}</p>
      <div className="project-card-footer">
        <span className="project-card-visibility">{project.visibility}</span>
        <span className="project-card-date">Создан: {formatDate(project.createdAt)}</span>
      </div>
    </div>
  );
}

